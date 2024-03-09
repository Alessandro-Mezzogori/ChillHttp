#include <chill_thread.h>


#pragma region Pipeline


errno_t noop_step(PipelineContext* context) {
	LOG_INFO("[PipelineHandler]");
	return next(context);
}

errno_t test_error_step(PipelineContext* context) {
	LOG_INFO("[PipelineHandler]");
	HttpRequest* request = context->request;
	HttpResponse* response = context->response;
	ConnectionData* connectionData = context->connectionData;

	// close connection if Connection: close header is present
	HashEntry* connection = hashtableLookup(request->headers, "TEST-ERROR");
	if (connection != NULL) {
		unsigned short error = (unsigned short) strtol(connection->value, NULL, 10);
		response->statusCode = error;
	}

	// if next step is present, call it
	return next(context);
}

errno_t handleConnectionHeader(PipelineContext* context) {
	LOG_INFO("[PipelineHandler]");
	HttpRequest* request = context->request;
	ConnectionData* connectionData = context->connectionData;

	// close connection if Connection: close header is present
	HashEntry* connection = hashtableLookup(request->headers, "Connection");
	if (connection != NULL && strcmp(connection->value, "close") == 0) {
		connectionData->connectionStatus = CONNECTION_STATUS_CLOSING;
		LOG_INFO("Socket {%d} closing connection", socket);
	}

	// if next step is present, call it
	return next(context);
}

errno_t handleRequestStep(PipelineContext* context) {
	LOG_INFO("[PipelineHandler]");
	const Config* config = context->config;
	HttpRequest* request = context->request;
	HttpResponse* response = context->response;
	size_t routesize = context->routesSize;
	Route* routes = context->routes;

	errno_t err = 0;

	err = handleError(config, request, response);
	if (err == 1) {
		err = handleRequest(routes, routesize, config, request, response);
		if(err != 0) {
			LOG_ERROR("Error while handling request: %d", err);
			return err;
		}
	}

	return next(context);
}

#pragma endregion

#pragma region Routes 

errno_t test_route(const Config* config, HttpRequest* request, HttpResponse* response) {
	LOG_INFO("[RouteHandler]");
	return setHttpResponse(response, request->version, 203, strdup("test endpoint"));
}

errno_t test_route2(const Config* config, HttpRequest* request, HttpResponse* response) {
	LOG_INFO("[RouteHandler]");
	return setHttpResponse(response, request->version, 203, strdup("test endpoint 2"));
}

errno_t serve_file(const Config* config, HttpRequest* request, HttpResponse* response) {
	LOG_INFO("[RouteHandler]");
	return serveFile(config->servingFolder, config->servingFolderLen, request, response);
}

#pragma endregion

DWORD threadFunction(void* lpParam) {
	PSOCKTD pdata = (PSOCKTD)lpParam;
	PMTData mtData = pdata->pmtData;
	int threadId = pdata->threadId;
	SOCKET socket = pdata->socket;

	LOG_INFO("Thread {%d} socket: {%d} initiliazed", threadId, socket);

	// solo un modo per autoallocare la memoria in maniera temporanea 
	PipelineStep steps[4];
	steps[0].name = "connection closer";
	steps[0].function = handleConnectionHeader;
	steps[0].next = &steps[1];

	steps[1].name = "test error";
	steps[1].function = test_error_step;
	steps[1].next = &steps[2];

	steps[2].name = "noop";
	steps[2].function = noop_step;
	steps[2].next = &steps[3];

	steps[3].name = "handle request";
	steps[3].function = handleRequestStep;
	steps[3].next = NULL;

	size_t routeSize = 3;
	Route routes[3];
	registerRoute(routes, "/test", HTTP_POST, test_route);
	registerRoute(routes + 1, "/test2", HTTP_POST, test_route2);
	registerRoute(routes + 2, "*", HTTP_GET, serve_file);

	do {
		HttpRequest request;
		errno_t err = recvRequest(socket, &request);
		if (err != 0) {
			LOG_ERROR("Error while receiving request: %d", err);
			break;
		}
			
		// read path
		HttpResponse response;
		errno_t responseCreateErr = createHttpResponse(&response);
		if(responseCreateErr != 0){
			// TODO short circuit with a stackalloced response 500
			LOG_ERROR("Error while creating response: %d", responseCreateErr);
			freeHttpRequest(&request);
			return 1;
		}

		PipelineContext context = {
			&request,
			&response,
			&pdata->connectionData,
			&mtData->config,
			&steps[0],
			routes,
			routeSize
		};

		errno_t pipelineError = startPipeline(&context);
		if(pipelineError != 0) {
			LOG_ERROR("Error while handling request: %d", pipelineError);
			// TODO short circuit with a stackalloced response 500 or some other error
		}

		size_t responseBufferLen = 0;
		char* responseBuffer;
		errno_t buildErr = buildHttpResponse(&response, &responseBuffer, &responseBufferLen);
		if (buildErr != 0) {
			LOG_ERROR("Error while building response: %d", buildErr);
			freeHttpRequest(&request);
			freeHttpResponse(&response);
			return 1;
		}

		int sendResult = send(socket, responseBuffer, (int) responseBufferLen, 0);
		if (sendResult == SOCKET_ERROR) {
			LOG_FATAL("Socket {%d} send failed: %d", socket, WSAGetLastError());
			closesocket(socket);

			free(responseBuffer);
			freeHttpRequest(&request);
			freeHttpResponse(&response);
			WSACleanup();
			return 1;
		}

		// LOG_INFO("Socket {%d} bytes sent: %d", socket, sendResult);
		// LOG_INFO("Socket:{%d} Message:\r\n%s", socket, responseBuffer);

		free(responseBuffer);
		freeHttpRequest(&request);
		freeHttpResponse(&response);
	} while (pdata->connectionData.connectionStatus = CONNECTION_STATUS_CONNECTED);

	if (shutdown(socket, SD_SEND) == SOCKET_ERROR) {
		LOG_FATAL("Socket {%d} Shutdown failed: %d", socket, WSAGetLastError());
		closesocket(socket);
		WSACleanup();
		return 1;
	}

	LOG_TRACE("Closing thread {%d} socket {%d}", threadId, socket);
	closesocket(socket);

	pdata->isActive = false;
	return 0;
}

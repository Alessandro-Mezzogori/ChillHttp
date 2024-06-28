#include <chill_thread.h>
#include <chill_lua_boolean_array.h>

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

	LOG_DEBUG("Registering routes...");
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
		LOG_DEBUG("Request rcved: %s", request.path);
			
		HttpResponse response;
		errno_t responseCreateErr = createHttpResponse(&response);
		if(responseCreateErr != 0){
			// TODO short circuit with a stackalloced response 500
			LOG_ERROR("Error while creating response: %d", responseCreateErr);
			freeHttpRequest(&request);
			return 1;
		}

		LOG_DEBUG("Setup pipeline context");
		PipelineContextInit context = {
			.request = &request,
			.response = &response,
			.connectionData = &pdata->connectionData,
			.config = &mtData->config,
			.routes = routes,
			.routesSize = routeSize,
		};

		LOG_INFO("Running request pipeline");
		errno_t pipelineError = runPipeline(&context);
		if(pipelineError != 0) {
			LOG_ERROR("Error while handling request: %d", pipelineError);
			// TODO short circuit with a stackalloced response 500 or some other error
		}

		LOG_DEBUG("Building http response");
		size_t responseBufferLen = 0;
		char* responseBuffer;
		errno_t buildErr = buildHttpResponse(&response, &responseBuffer, &responseBufferLen);
		if (buildErr != 0) {
			LOG_ERROR("Error while building response: %d", buildErr);
			freeHttpRequest(&request);
			freeHttpResponse(&response);
			return 1;
		}

		LOG_DEBUG("Sending http response");
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

		free(responseBuffer);
		freeHttpRequest(&request);
		freeHttpResponse(&response);
	} while (pdata->connectionData.connectionStatus == CONNECTION_STATUS_CONNECTED);

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

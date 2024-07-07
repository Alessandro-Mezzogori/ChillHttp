#include <chill_thread_old.h>
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

void task_function(void* lpParam) {
	TaskContext* data = (TaskContext*)lpParam;
	Config* config = data->config;
	SOCKET socket = data->httpcontext.socket;
	ConnectionData* connectionData = &data->httpcontext.connectionData;

	LOG_DEBUG("Registering routes...");
	size_t routeSize = 3;
	Route routes[3];
	registerRoute(routes, "/test", HTTP_POST, test_route);
	registerRoute(routes + 1, "/test2", HTTP_POST, test_route2);
	registerRoute(routes + 2, "*", HTTP_GET, serve_file);

	// TODO remove do while in task worker function
	// to do so we need a global registry for the current open connections
	// probably the best way is having a concorrent map and a thread updating some info about the connection
	// life the connection lifetime ecc... or if it is marked as a bad actor and forcibly closed
	// ConnectionData could be a good starting point for the registry

	do {
		int loop_cleanup = 0;
		HttpRequest request;
		HttpResponse response;
		size_t responseBufferLen = 0;
		char* responseBuffer = NULL;

		errno_t err = recvRequest(socket, &request);
		if (err != 0) {
			LOG_ERROR("Error while receiving request: %d", err);
			connectionData->connectionStatus = CONNECTION_STATUS_ABORTING;
			break;
		}
		loop_cleanup = 1; // request rcved
		LOG_DEBUG("Request rcved: %s", request.path);
			
		errno_t responseCreateErr = createHttpResponse(&response);
		if(responseCreateErr != 0){
			// TODO short circuit with a stackalloced response 500
			LOG_ERROR("Error while creating response: %d", responseCreateErr);
			connectionData->connectionStatus = CONNECTION_STATUS_ABORTING;
			goto _loop_cleanup;
		}
		loop_cleanup = 2; // response created

		LOG_DEBUG("Setup pipeline context");
		PipelineContextInit context = {
			.request = &request,
			.response = &response,
			.connectionData = connectionData,
			.config = config,
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
		errno_t buildErr = buildHttpResponse(&response, &responseBuffer, &responseBufferLen);
		if (buildErr != 0) {
			LOG_ERROR("Error while building response: %d", buildErr);
			connectionData->connectionStatus = CONNECTION_STATUS_ABORTING;
			goto _loop_cleanup;
		}
		loop_cleanup = 3; // built http response

		LOG_DEBUG("Sending http response");
		int sendResult = send(socket, responseBuffer, (int) responseBufferLen, 0);
		if (sendResult == SOCKET_ERROR) {
			LOG_FATAL("Socket {%d} send failed: %d", socket, WSAGetLastError());
			loop_cleanup = 10000; // socket error
			connectionData->connectionStatus = CONNECTION_STATUS_ABORTING;
			goto _loop_cleanup;
		}

_loop_cleanup:
	LOG_WARN("Loop cleanup task function");
	switch (loop_cleanup) {
		case 10000:
			closesocket(socket);
			socket = NULL;
			WSACleanup();
		case 3:
			free(responseBuffer);
		case 2:
			freeHttpResponse(&response);
		case 1:
			freeHttpRequest(&request);
		default:
			break;
	}
	} while (connectionData->connectionStatus == CONNECTION_STATUS_CONNECTED);

	if (shutdown(socket, SD_SEND) == SOCKET_ERROR) {
		LOG_FATAL("Socket {%d} Shutdown failed: %d", socket, WSAGetLastError());
	}

	data->httpcontext.isActive = false;
	closesocket(socket);
	free(data); // TODO better free of task context
	//WSACleanup(); // TODO idk if this is good here
}

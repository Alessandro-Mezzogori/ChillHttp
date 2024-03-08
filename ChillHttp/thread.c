#include "thread.h"


errno_t buildResponse(HashTable headers, char* body) {
	return 0;
}

DWORD threadFunction(void* lpParam) {
	PSOCKTD pdata = (PSOCKTD)lpParam;
	PMTData mtData = pdata->pmtData;
	int threadId = pdata->threadId;
	SOCKET socket = pdata->socket;

	LOG_INFO("Thread {%d} socket: {%d} initiliazed", threadId, socket);

	do {
		HttpRequest request;
		errno_t err = recvRequest(socket, &request);
		if (err != 0) {
			LOG_ERROR("Error while receiving request: %d", err);
			break;
		}
			
		// read path
		HttpResponse* response;
		errno_t responseCreateErr = createHttpResponse(&response);
		if(responseCreateErr != 0){
			// TODO short circuit with a stackalloced response 500
			LOG_ERROR("Error while creating response: %d", responseCreateErr);
			freeHttpRequest(&request);
			return 1;
		}

		HashEntry* connection = hashtableLookup(request.headers, "Connection");
		if (connection != NULL && strcmp(connection->value, "close") == 0) {
			pdata->connectionStatus = CONNECTION_STATUS_CLOSING;
			LOG_INFO("Socket {%d} closing connection", socket);
		}

		errno_t handleErr = handleRequest(&mtData->config, &request, response);
		if(handleErr != 0) {
			LOG_ERROR("Error while handling request: %d", handleErr);
			freeHttpRequest(&request);
			freeHttpResponse(response);
			return 1;
		}

		size_t responseBufferLen = 0;
		char* responseBuffer;
		errno_t buildErr = buildHttpResponse(response, &responseBuffer, &responseBufferLen);
		if (buildErr != 0) {
			LOG_ERROR("Error while building response: %d", buildErr);
			freeHttpRequest(&request);
			freeHttpResponse(response);
			return 1;
		}

		int sendResult = send(socket, responseBuffer, responseBufferLen, 0);
		if (sendResult == SOCKET_ERROR) {
			LOG_FATAL("Socket {%d} send failed: %d", socket, WSAGetLastError());
			closesocket(socket);

			free(responseBuffer);
			freeHttpRequest(&request);
			freeHttpResponse(response);
			WSACleanup();
			return 1;
		}

		LOG_INFO("Socket {%d} bytes sent: %d", socket, sendResult);
		LOG_INFO("Socket:{%d} Message:\r\n%s", socket, responseBuffer);

		free(responseBuffer);
		freeHttpRequest(&request);
		freeHttpResponse(response);
	} while (pdata->connectionStatus = CONNECTION_STATUS_CONNECTED);

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

#pragma once

typedef enum {
	CONNECTION_STATUS_UNKNOWN	= 0,
	CONNECTION_STATUS_CONNECTED = 1,
	CONNECTION_STATUS_CLOSING	= 2,
	CONNECTION_STATUS_CLOSED	= 3
} ConnectionStatus;

typedef struct {
	ConnectionStatus connectionStatus;
} ConnectionData;

#pragma once

typedef enum {
	CONNECTION_STATUS_UNKNOWN,
	CONNECTION_STATUS_CONNECTED,
	CONNECTION_STATUS_CLOSING,
	CONNECTION_STATUS_CLOSED
} ConnectionStatus;

typedef struct {
	ConnectionStatus connectionStatus;
} ConnectionData;

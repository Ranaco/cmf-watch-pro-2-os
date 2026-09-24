/* SPDX-License-Identifier: Apache-2.0 */
#include "transport/transport.h"

#include "protocol/watch_protocol.h"
#include <errno.h>
#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/random/random.h>

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(watch_transport);

#define HOST_PORT 4660
#define RECONNECT_INTERVAL_MS 1000

static int socket_fd = -1;
static int64_t next_connect_at;
static uint32_t next_message_id = 1;
static struct watch_frame_decoder decoder;
static char session_id[37];
static transport_message_handler_t runtime_message_handler;
static transport_event_handler_t runtime_event_handler;
static void *runtime_context;

static void disconnect(void)
{
	if (socket_fd >= 0) {
		zsock_close(socket_fd);
		socket_fd = -1;
		if (runtime_event_handler != NULL) {
			runtime_event_handler(TRANSPORT_EVENT_DISCONNECTED, runtime_context);
		}
	}
	decoder.length = 0;
	next_connect_at = k_uptime_get() + RECONNECT_INTERVAL_MS;
}

static int send_bytes(const char *bytes, size_t length)
{
	size_t sent = 0;
	while (sent < length) {
		int result = zsock_send(socket_fd, bytes + sent, length - sent, 0);
		if (result <= 0) {
			return -EIO;
		}
		sent += (size_t)result;
	}
	return 0;
}

static int send_bootstrap(void)
{
	char frame[512];
	int length = snprintk(frame, sizeof(frame),
		"{\"version\":1,\"type\":\"hello\",\"id\":%u,\"payload\":{"
		"\"device_id\":\"cmf-watch-pro-2-sim\",\"session_id\":\"%s\","
		"\"runtime_version\":\"0.1.0\",\"protocol_versions\":[1],\"simulator\":true}}\n",
		next_message_id++, session_id);
	if (length <= 0 || (size_t)length >= sizeof(frame) || send_bytes(frame, (size_t)length) < 0) {
		return -EIO;
	}
	return transport_request_sync(0);
}

static bool is_command(enum watch_message_type type)
{
	return type == WATCH_MESSAGE_REFRESH || type == WATCH_MESSAGE_APP_OPEN ||
	       type == WATCH_MESSAGE_SHOW_TOAST || type == WATCH_MESSAGE_SHOW_DIALOG ||
	       type == WATCH_MESSAGE_CACHE_INVALIDATE;
}

static int send_unsupported_result(uint32_t reply_to)
{
	char frame[256];
	int length = snprintk(frame, sizeof(frame),
		"{\"version\":1,\"type\":\"command_result\",\"id\":%u,\"reply_to\":%u,"
		"\"payload\":{\"status\":\"rejected\",\"code\":\"unsupported\"}}\n",
		next_message_id++, reply_to);
	return (length > 0 && (size_t)length < sizeof(frame)) ? send_bytes(frame, (size_t)length) : -EIO;
}

static void connect_if_due(void)
{
	if (socket_fd >= 0 || k_uptime_get() < next_connect_at) {
		return;
	}
	int candidate = zsock_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (candidate < 0) {
		next_connect_at = k_uptime_get() + RECONNECT_INTERVAL_MS;
		return;
	}
	struct sockaddr_in address = {
		.sin_family = AF_INET,
		.sin_port = htons(HOST_PORT),
	};
	zsock_inet_pton(AF_INET, "127.0.0.1", &address.sin_addr);
	if (zsock_connect(candidate, (struct sockaddr *)&address, sizeof(address)) < 0) {
		zsock_close(candidate);
		next_connect_at = k_uptime_get() + RECONNECT_INTERVAL_MS;
		return;
	}
	socket_fd = candidate;
	LOG_INF("Connected to host at 127.0.0.1:%d", HOST_PORT);
	if (runtime_event_handler != NULL) {
		runtime_event_handler(TRANSPORT_EVENT_CONNECTED, runtime_context);
	}
	if (send_bootstrap() < 0) {
		disconnect();
	}
}

static int handle_message(const struct watch_protocol_message *message, void *context)
{
	ARG_UNUSED(context);
	LOG_INF("Protocol: %s id=%u", watch_message_type_name(message->type), message->id);
	if (is_command(message->type)) {
		return send_unsupported_result(message->id);
	}
	return runtime_message_handler != NULL ? runtime_message_handler(message, runtime_context) : 0;
}

void transport_init(transport_message_handler_t message_handler,
		    transport_event_handler_t event_handler, void *context)
{
	runtime_message_handler = message_handler;
	runtime_event_handler = event_handler;
	runtime_context = context;
	uint32_t random[4] = {sys_rand32_get(), sys_rand32_get(), sys_rand32_get(), sys_rand32_get()};
	random[1] = (random[1] & 0xffff0fffU) | 0x00004000U;
	random[2] = (random[2] & 0x3fffffffU) | 0x80000000U;
	snprintk(session_id, sizeof(session_id), "%08x-%04x-%04x-%04x-%04x%08x",
		random[0], random[1] >> 16, random[1] & 0xffffU, random[2] >> 16,
		random[2] & 0xffffU, random[3]);
	next_connect_at = 0;
	connect_if_due();
}

void transport_poll(void)
{
	connect_if_due();
	if (socket_fd < 0) {
		return;
	}
	uint8_t bytes[1024];
	int count = zsock_recv(socket_fd, bytes, sizeof(bytes), ZSOCK_MSG_DONTWAIT);
	if (count > 0) {
		int result = watch_frame_decoder_push(&decoder, bytes, (size_t)count,
					      handle_message, NULL);
		if (result < 0) {
			LOG_WRN("Protocol error: %d", result);
			disconnect();
		}
	} else if (count == 0 || (errno != EAGAIN && errno != EWOULDBLOCK)) {
		LOG_INF("Host disconnected");
		disconnect();
	}
}

bool transport_connected(void)
{
	return socket_fd >= 0;
}

int transport_request_sync(uint64_t last_revision)
{
	if (socket_fd < 0) {
		return -ENOTCONN;
	}
	char frame[192];
	int length = snprintk(frame, sizeof(frame),
		"{\"version\":1,\"type\":\"sync_request\",\"id\":%u,"
		"\"payload\":{\"last_revision\":%llu}}\n", next_message_id++,
		(unsigned long long)last_revision);
	return (length > 0 && (size_t)length < sizeof(frame)) ? send_bytes(frame, (size_t)length) : -EIO;
}

#include "EmbeddedOSSupportHost.h"
#include <sys/socket.h>
#include <CoreFoundation/CoreFoundation.h>

#define LOG(fmt, ...)

void eos_message_destroy(eos_message_t msg) {
	if (msg != NULL) {
		CFRelease(msg);
	}
}

uint32_t _eos_message_calculcate_crc(const uint8_t* buffer, uint32_t len) {
  uint32_t result = 0;
  for (uint32_t i = 0; i != len; ++i) {
  	result += buffer[i];
  }
  return result;
}

// _eos_message_recv in apple terms
// 1 on success, 0 on failure
static int recv_all(int socket, void* buffer, size_t length) {
	ssize_t recvd = 0;

	while (length > 0) {
		recvd = recv(socket, buffer, length, 0);
		if (recvd <= 0) {
			LOG("recv failed: %s", strerror(errno()));
			return 0;
		}

		assert(length >= recvd);

		buffer = (void*) ((uintptr_t)buffer + recvd);
		length -= recvd;
	}

	return 1;
}

eos_message_t eos_message_receive(eos_connection_t conn) {
	struct eos_message_serialized message = {};
	int rv = 0;
	eos_message_t result = NULL;
	void* payload_buf = NULL;
	CFDataRef payload_dataref = NULL;

	assert(conn >= 0);

	rv = recv_all((int) conn, &message.raw_header_len, sizeof(message.raw_header_len));
	if (!rv) {
		LOG("Cant recv incoming header length");
		goto endret;
	}

	// Not present in older versions (i.e. on first gen iBridge on touchbars)
	if (message.raw_header_len > sizeof(message.header)) {
		LOG("Header length too large");
		goto endret;
	}

	rv = recv_all((int) conn, &message.header, message.raw_header_len);
	if (!rv) {
		LOG("Cant recv incoming header");
		goto endret;
	}

	// 0 isn't valid either since it overflows, lol
	if (message.header.eos_msg.payload_len - 1 >= MAX_PAYLOAD_LEN) {
		LOG("Invalid payload length: %d", message.header.eos_msg.payload_len);
		goto endret;
	}

	payload_buf = calloc(1, message.header.eos_msg.payload_len);
	if (payload_buf == NULL) {
		LOG("calloc failed");
		goto endret;
	}

	rv = recv_all((int) conn, payload_buf, message.header.eos_msg.payload_len);
	if (!rv) {
		LOG("Cant recv payload");
		goto endret;
	}

	if (_eos_message_calculcate_crc(payload_buf, message.header.eos_msg.payload_len) != message.header.eos_msg.crc) {
		LOG("Invalid crc");
		goto endret;
	}

	payload_dataref = CFDataCreateWithBytesNoCopy(kCFAllocatorDefault, payload_buf, message.header.eos_msg.payload_len, kCFAllocatorNull);
	if (payload_dataref == NULL) {
		LOG("Cant create CFDataRef for payload");
		goto endret;
	}

	result = CFPropertyListCreateWithData(kCFAllocatorDefault, payload_dataref, kCFPropertyListMutableContainers, NULL, NULL);

	if (result == NULL) {
		LOG("Cant deserialize incoming message");
	}

endret:
	if (payload_dataref) CFRelease(payload_dataref);
	free(payload_buf);
	return result;
}
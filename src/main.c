#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <switch.h>
#include <switch/services/bsd.h>
#include <threads.h>

void __appExit(void) {
	bsdExit();
}

static const char HTTP_RESPONSE[] =
	"HTTP/1.0 200 OK\r\n"
	"Content-Length: 2\r\n"
	"X-Organization: Nintendo\r\n"
	"\r\n"
	"ok";

enum {
	ADDRESS_LENGTH = sizeof(struct sockaddr_in),
	BUFFER_LENGTH = 0x10000,
	HTTP_LENGTH = sizeof(HTTP_RESPONSE) - 1,
	OPTION_LENGTH = sizeof(u32),
	PAGE_LENGTH = 0x1000,
};

static alignas(PAGE_LENGTH) u8 transfer_buffer[BUFFER_LENGTH];
static int g_bsd_ready = 0;

static void sleep_sec(int sec) {
	thrd_sleep(&(const struct timespec){.tv_sec = sec}, nullptr);
}

static bool ensure_bsd(void) {
	if (g_bsd_ready) {
		return true;
	}

	Result rc = smInitialize();
	if (R_FAILED(rc)) {
		return false;
	}

	BsdInitConfig cfg = *bsdGetDefaultInitConfig();
	cfg.version = 1;
	cfg.tmem_buffer = transfer_buffer;
	cfg.tmem_buffer_size = sizeof(transfer_buffer);
	cfg.tcp_tx_buf_size = 0x2000;
	cfg.tcp_rx_buf_size = 0x2000;
	cfg.tcp_tx_buf_max_size = 0;
	cfg.tcp_rx_buf_max_size = 0;
	cfg.udp_tx_buf_size = 0;
	cfg.udp_rx_buf_size = 0;
	cfg.sb_efficiency = 1;

	rc = bsdInitialize(&cfg, 2, BsdServiceType_Auto);
	smExit();
	if (R_FAILED(rc)) {
		return false;
	}

	g_bsd_ready = 1;
	return true;
}

int main(void) {
	/* Do not abort in __appInit: at boot2, bsd may not be ready yet. */
	while (true) {
		if (!ensure_bsd()) {
			sleep_sec(2);
			continue;
		}

		const int server = bsdSocket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (server < 0) {
			sleep_sec(1);
			continue;
		}

		u32 yes = 1;
		struct sockaddr_in address = {
			.sin_len = ADDRESS_LENGTH,
			.sin_family = AF_INET,
			.sin_port = htons(80),
		};

		if (inet_pton(AF_INET, "127.0.0.1", &address.sin_addr) != 1 ||
			bsdSetSockOpt(server, SOL_SOCKET, SO_REUSEADDR, &yes, OPTION_LENGTH) ||
			bsdBind(server, (const struct sockaddr *)&address, ADDRESS_LENGTH) ||
			bsdListen(server, 8)) {
			bsdClose(server);
			sleep_sec(1);
			continue;
		}

		while (true) {
			const int client = bsdAccept(server, nullptr, nullptr);
			if (client < 0) {
				break;
			}

			int cursor = 0;
			while (cursor < HTTP_LENGTH) {
				const ssize_t sent = bsdSend(client, HTTP_RESPONSE + cursor, (size_t)(HTTP_LENGTH - cursor), 0);
				if (sent <= 0) {
					break;
				}
				cursor += (int)sent;
			}
			bsdClose(client);
		}

		bsdClose(server);
		sleep_sec(1);
	}

	return 0;
}

void __appInit(void) {
	/* Intentionally empty: services initialized lazily in main(). */
}

extern void *fake_heap_start;
extern void *fake_heap_end;
static alignas(PAGE_LENGTH) u8 fake_heap[PAGE_LENGTH * 8];

void __libnx_initheap(void) {
	fake_heap_start = fake_heap;
	fake_heap_end = fake_heap + sizeof(fake_heap);
}

const u32 __nx_applet_type = AppletType_None;
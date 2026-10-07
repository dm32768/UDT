// Sends 8 MiB of a known pattern through a UDT stream socket over loopback
// and checks every byte on the receiving side. A second transfer runs with
// the sender capped at 4 MB/s (UDT_MAXBW) and must take about two seconds:
// the cap is enforced by the packet-pacing timer, so a timer running at the
// wrong rate shows up here.
#include <udt.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <thread>
#include <vector>

static const int kPort = 19313;
static const size_t kTotal = 8u << 20;
static const int64_t kCapBytesPerSec = 4000000;

static unsigned char pattern(size_t i) { return (unsigned char)((i * 131 + (i >> 8)) & 0xff); }

static bool recv_all(UDTSOCKET s, size_t total)
{
   std::vector<char> buf(64 << 10);
   size_t got = 0;
   while (got < total)
   {
      int n = UDT::recv(s, buf.data(), (int)std::min(buf.size(), total - got), 0);
      if (n == UDT::ERROR)
      {
         fprintf(stderr, "recv: %s\n", UDT::getlasterror().getErrorMessage());
         return false;
      }
      for (int i = 0; i < n; i++)
         if ((unsigned char)buf[i] != pattern(got + i))
         {
            fprintf(stderr, "byte %zu differs\n", got + i);
            return false;
         }
      got += n;
   }
   return true;
}

// Runs one transfer; returns the seconds it took, or a negative number on
// failure. port is distinct per run so that a lingering socket cannot block.
static double transfer(int port, int64_t maxbw)
{
   UDTSOCKET serv = UDT::socket(AF_INET, SOCK_STREAM, 0);
   sockaddr_in addr;
   memset(&addr, 0, sizeof addr);
   addr.sin_family = AF_INET;
   addr.sin_port = htons(port);
   addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
   if (UDT::bind(serv, (sockaddr*)&addr, sizeof addr) == UDT::ERROR || UDT::listen(serv, 1) == UDT::ERROR)
   {
      fprintf(stderr, "listen: %s\n", UDT::getlasterror().getErrorMessage());
      return -1;
   }

   bool ok = false;
   std::thread receiver([&] {
      int len = sizeof addr;
      UDTSOCKET c = UDT::accept(serv, (sockaddr*)&addr, &len);
      if (c == UDT::INVALID_SOCK)
      {
         fprintf(stderr, "accept: %s\n", UDT::getlasterror().getErrorMessage());
         return;
      }
      ok = recv_all(c, kTotal);
      UDT::close(c);
   });

   UDTSOCKET cli = UDT::socket(AF_INET, SOCK_STREAM, 0);
   if (maxbw > 0)
      UDT::setsockopt(cli, 0, UDT_MAXBW, &maxbw, sizeof maxbw);
   if (UDT::connect(cli, (sockaddr*)&addr, sizeof addr) == UDT::ERROR)
   {
      fprintf(stderr, "connect: %s\n", UDT::getlasterror().getErrorMessage());
      return -1;
   }
   std::vector<char> out(kTotal);
   for (size_t i = 0; i < kTotal; i++)
      out[i] = (char)pattern(i);

   auto start = std::chrono::steady_clock::now();
   size_t sent = 0;
   while (sent < kTotal)
   {
      int n = UDT::send(cli, out.data() + sent, (int)(kTotal - sent), 0);
      if (n == UDT::ERROR)
      {
         fprintf(stderr, "send: %s\n", UDT::getlasterror().getErrorMessage());
         return -1;
      }
      sent += n;
   }
   receiver.join();
   double secs = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();

   UDT::close(cli);
   UDT::close(serv);
   return ok ? secs : -1;
}

int main()
{
   UDT::startup();
   int failures = 0;

   double full = transfer(kPort, 0);
   if (full < 0)
      failures++;
   printf(full < 0 ? "loopback: FAILED\n" : "loopback: %zu bytes intact in %.2f s\n", kTotal, full);

   double capped = transfer(kPort + 1, kCapBytesPerSec);
   double expected = (double)kTotal / kCapBytesPerSec;
   // The pacing is per packet and the receiver's window adds slack, so allow
   // a wide band around the nominal time; a timer off by a factor fails it.
   bool rate_ok = capped > 0 && capped > expected * 0.7 && capped < expected * 1.6;
   if (!rate_ok)
      failures++;
   printf("capped at %lld B/s: %.2f s (nominal %.2f s) %s\n", (long long)kCapBytesPerSec, capped, expected,
          rate_ok ? "ok" : "FAILED");

   UDT::cleanup();
   return failures ? 1 : 0;
}

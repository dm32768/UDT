// Sends 8 MiB of a known pattern through a UDT stream socket over loopback
// and checks every byte on the receiving side.
#include <udt.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <thread>
#include <vector>

static const int kPort = 19313;
static const size_t kTotal = 8u << 20;

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

int main()
{
   UDT::startup();

   UDTSOCKET serv = UDT::socket(AF_INET, SOCK_STREAM, 0);
   sockaddr_in addr;
   memset(&addr, 0, sizeof addr);
   addr.sin_family = AF_INET;
   addr.sin_port = htons(kPort);
   addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
   if (UDT::bind(serv, (sockaddr*)&addr, sizeof addr) == UDT::ERROR || UDT::listen(serv, 1) == UDT::ERROR)
   {
      fprintf(stderr, "listen: %s\n", UDT::getlasterror().getErrorMessage());
      return 2;
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
   if (UDT::connect(cli, (sockaddr*)&addr, sizeof addr) == UDT::ERROR)
   {
      fprintf(stderr, "connect: %s\n", UDT::getlasterror().getErrorMessage());
      return 2;
   }
   std::vector<char> out(kTotal);
   for (size_t i = 0; i < kTotal; i++)
      out[i] = (char)pattern(i);
   size_t sent = 0;
   while (sent < kTotal)
   {
      int n = UDT::send(cli, out.data() + sent, (int)(kTotal - sent), 0);
      if (n == UDT::ERROR)
      {
         fprintf(stderr, "send: %s\n", UDT::getlasterror().getErrorMessage());
         return 2;
      }
      sent += n;
   }

   receiver.join();
   UDT::close(cli);
   UDT::close(serv);
   UDT::cleanup();
   printf(ok ? "loopback: %zu bytes intact\n" : "loopback: FAILED\n", kTotal);
   return ok ? 0 : 1;
}

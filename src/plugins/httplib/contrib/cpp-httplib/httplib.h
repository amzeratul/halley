//
//  httplib.h
//
//  Copyright (c) 2022 Yuji Hirose. All rights reserved.
//  MIT License
//

#ifndef CPPHTTPLIB_HTTPLIB_H
#define CPPHTTPLIB_HTTPLIB_H

#define CPPHTTPLIB_VERSION "0.12.1"

/*
 * Configuration
 */

#ifndef CPPHTTPLIB_KEEPALIVE_TIMEOUT_SECOND
#define CPPHTTPLIB_KEEPALIVE_TIMEOUT_SECOND 5
#endif

#ifndef CPPHTTPLIB_KEEPALIVE_MAX_COUNT
#define CPPHTTPLIB_KEEPALIVE_MAX_COUNT 5
#endif

#ifndef CPPHTTPLIB_CONNECTION_TIMEOUT_SECOND
#define CPPHTTPLIB_CONNECTION_TIMEOUT_SECOND 300
#endif

#ifndef CPPHTTPLIB_CONNECTION_TIMEOUT_USECOND
#define CPPHTTPLIB_CONNECTION_TIMEOUT_USECOND 0
#endif

#ifndef CPPHTTPLIB_READ_TIMEOUT_SECOND
#define CPPHTTPLIB_READ_TIMEOUT_SECOND 5
#endif

#ifndef CPPHTTPLIB_READ_TIMEOUT_USECOND
#define CPPHTTPLIB_READ_TIMEOUT_USECOND 0
#endif

#ifndef CPPHTTPLIB_WRITE_TIMEOUT_SECOND
#define CPPHTTPLIB_WRITE_TIMEOUT_SECOND 5
#endif

#ifndef CPPHTTPLIB_WRITE_TIMEOUT_USECOND
#define CPPHTTPLIB_WRITE_TIMEOUT_USECOND 0
#endif

#ifndef CPPHTTPLIB_IDLE_INTERVAL_SECOND
#define CPPHTTPLIB_IDLE_INTERVAL_SECOND 0
#endif

#ifndef CPPHTTPLIB_IDLE_INTERVAL_USECOND
#ifdef _WIN32
#define CPPHTTPLIB_IDLE_INTERVAL_USECOND 10000
#else
#define CPPHTTPLIB_IDLE_INTERVAL_USECOND 0
#endif
#endif

#ifndef CPPHTTPLIB_REQUEST_URI_MAX_LENGTH
#define CPPHTTPLIB_REQUEST_URI_MAX_LENGTH 8192
#endif

#ifndef CPPHTTPLIB_HEADER_MAX_LENGTH
#define CPPHTTPLIB_HEADER_MAX_LENGTH 8192
#endif

#ifndef CPPHTTPLIB_REDIRECT_MAX_COUNT
#define CPPHTTPLIB_REDIRECT_MAX_COUNT 20
#endif

#ifndef CPPHTTPLIB_MULTIPART_FORM_DATA_FILE_MAX_COUNT
#define CPPHTTPLIB_MULTIPART_FORM_DATA_FILE_MAX_COUNT 1024
#endif

#ifndef CPPHTTPLIB_PAYLOAD_MAX_LENGTH
#define CPPHTTPLIB_PAYLOAD_MAX_LENGTH ((std::numeric_limits<size_t>::max)())
#endif

#ifndef CPPHTTPLIB_FORM_URL_ENCODED_PAYLOAD_MAX_LENGTH
#define CPPHTTPLIB_FORM_URL_ENCODED_PAYLOAD_MAX_LENGTH 8192
#endif

#ifndef CPPHTTPLIB_TCP_NODELAY
#define CPPHTTPLIB_TCP_NODELAY false
#endif

#ifndef CPPHTTPLIB_RECV_BUFSIZ
#define CPPHTTPLIB_RECV_BUFSIZ size_t(4096u)
#endif

#ifndef CPPHTTPLIB_COMPRESSION_BUFSIZ
#define CPPHTTPLIB_COMPRESSION_BUFSIZ size_t(16384u)
#endif

#ifndef CPPHTTPLIB_THREAD_POOL_COUNT
#define CPPHTTPLIB_THREAD_POOL_COUNT                                           \
  ((std::max)(8u, std::thread::hardware_concurrency() > 0                      \
                      ? std::thread::hardware_concurrency() - 1                \
                      : 0))
#endif

#ifndef CPPHTTPLIB_RECV_FLAGS
#define CPPHTTPLIB_RECV_FLAGS 0
#endif

#ifndef CPPHTTPLIB_SEND_FLAGS
#define CPPHTTPLIB_SEND_FLAGS 0
#endif

#ifndef CPPHTTPLIB_LISTEN_BACKLOG
#define CPPHTTPLIB_LISTEN_BACKLOG 5
#endif

/*
 * Headers
 */

#ifdef _WIN32
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif //_CRT_SECURE_NO_WARNINGS

#ifndef _CRT_NONSTDC_NO_DEPRECATE
#define _CRT_NONSTDC_NO_DEPRECATE
#endif //_CRT_NONSTDC_NO_DEPRECATE

#if defined(_MSC_VER)
#if _MSC_VER < 1900
#error Sorry, Visual Studio versions prior to 2015 are not supported
#endif

#pragma comment(lib, "ws2_32.lib")

#ifdef _WIN64
using ssize_t = __int64;
#else
using ssize_t = long;
#endif
#endif // _MSC_VER

#ifndef S_ISREG
#define S_ISREG(m) (((m)&S_IFREG) == S_IFREG)
#endif // S_ISREG

#ifndef S_ISDIR
#define S_ISDIR(m) (((m)&S_IFDIR) == S_IFDIR)
#endif // S_ISDIR

#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX

#include <io.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#ifndef WSA_FLAG_NO_HANDLE_INHERIT
#define WSA_FLAG_NO_HANDLE_INHERIT 0x80
#endif

#ifndef strcasecmp
#define strcasecmp _stricmp
#endif // strcasecmp

using socket_t = SOCKET;
#ifdef CPPHTTPLIB_USE_POLL
#define poll(fds, nfds, timeout) WSAPoll(fds, nfds, timeout)
#endif

#else // not _WIN32

#include <arpa/inet.h>
#ifndef _AIX
#include <ifaddrs.h>
#endif
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#ifdef __linux__
#include <resolv.h>
#endif
#include <netinet/tcp.h>
#ifdef CPPHTTPLIB_USE_POLL
#include <poll.h>
#endif
#include <csignal>
#include <pthread.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

using socket_t = int;
#ifndef INVALID_SOCKET
#define INVALID_SOCKET (-1)
#endif
#endif //_WIN32

#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <cctype>
#include <climits>
#include <condition_variable>
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <random>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <thread>

#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
#ifdef _WIN32
#include <wincrypt.h>

// these are defined in wincrypt.h and it breaks compilation if BoringSSL is
// used
#undef X509_NAME
#undef X509_CERT_PAIR
#undef X509_EXTENSIONS
#undef PKCS7_SIGNER_INFO

#ifdef _MSC_VER
#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "cryptui.lib")
#endif
#elif defined(__APPLE__) // _WIN32
#include <CoreFoundation/CoreFoundation.h>
#include <Security/Security.h>
#endif // __APPLE__

#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/ssl.h>
#include <openssl/x509v3.h>

#if defined(_WIN32) && defined(OPENSSL_USE_APPLINK)
#include <openssl/applink.c>
#endif

#include <iostream>
#include <sstream>

#if OPENSSL_VERSION_NUMBER < 0x1010100fL
#error Sorry, OpenSSL versions prior to 1.1.1 are not supported
#elif OPENSSL_VERSION_NUMBER < 0x30000000L
#define SSL_get1_peer_certificate SSL_get_peer_certificate
#endif

#endif

#ifdef CPPHTTPLIB_ZLIB_SUPPORT
#include <zlib.h>
#endif

#ifdef CPPHTTPLIB_BROTLI_SUPPORT
#include <brotli/decode.h>
#include <brotli/encode.h>
#endif

/*
 * Declaration
 */
namespace httplib {

namespace detail {

/*
 * Backport std::make_unique from C++14.
 *
 * NOTE: This code came up with the following stackoverflow post:
 * https://stackoverflow.com/questions/10149840/c-arrays-and-make-unique
 *
 */

template <class T, class... Args>
typename std::enable_if<!std::is_array<T>::value, std::unique_ptr<T>>::type
make_unique(Args &&...args) {
  return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

template <class T>
typename std::enable_if<std::is_array<T>::value, std::unique_ptr<T>>::type
make_unique(std::size_t n) {
  typedef typename std::remove_extent<T>::type RT;
  return std::unique_ptr<T>(new RT[n]);
}

struct ci {
  bool operator()(const std::string &s1, const std::string &s2) const {
    return std::lexicographical_compare(s1.begin(), s1.end(), s2.begin(),
                                        s2.end(),
                                        [](unsigned char c1, unsigned char c2) {
                                          return ::tolower(c1) < ::tolower(c2);
                                        });
  }
};

// This is based on
// "http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2014/n4189".

template <typename EF> struct scope_exit {
  explicit scope_exit(EF &&f)
      : exit_function(std::move(f)), execute_on_destruction{true} {}

  scope_exit(scope_exit &&rhs)
      : exit_function(std::move(rhs.exit_function)),
        execute_on_destruction{rhs.execute_on_destruction} {
    rhs.release();
  }

  ~scope_exit() {
    if (execute_on_destruction) { this->exit_function(); }
  }

  void release() { this->execute_on_destruction = false; }

private:
  scope_exit(const scope_exit &) = delete;
  void operator=(const scope_exit &) = delete;
  scope_exit &operator=(scope_exit &&) = delete;

  EF exit_function;
  bool execute_on_destruction;
};

} // namespace detail

using Headers = std::multimap<std::string, std::string, detail::ci>;

using Params = std::multimap<std::string, std::string>;
using Match = std::smatch;

using Progress = std::function<bool(uint64_t current, uint64_t total)>;

struct Response;
using ResponseHandler = std::function<bool(const Response &response)>;

struct MultipartFormData {
  std::string name;
  std::string content;
  std::string filename;
  std::string content_type;
};
using MultipartFormDataItems = std::vector<MultipartFormData>;
using MultipartFormDataMap = std::multimap<std::string, MultipartFormData>;

class DataSink {
public:
  DataSink() : os(&sb_), sb_(*this) {}

  DataSink(const DataSink &) = delete;
  DataSink &operator=(const DataSink &) = delete;
  DataSink(DataSink &&) = delete;
  DataSink &operator=(DataSink &&) = delete;

  std::function<bool(const char *data, size_t data_len)> write;
  std::function<void()> done;
  std::function<void(const Headers &trailer)> done_with_trailer;
  std::ostream os;

private:
  class data_sink_streambuf : public std::streambuf {
  public:
    explicit data_sink_streambuf(DataSink &sink) : sink_(sink) {}

  protected:
    std::streamsize xsputn(const char *s, std::streamsize n) {
      sink_.write(s, static_cast<size_t>(n));
      return n;
    }

  private:
    DataSink &sink_;
  };

  data_sink_streambuf sb_;
};

using ContentProvider =
    std::function<bool(size_t offset, size_t length, DataSink &sink)>;

using ContentProviderWithoutLength =
    std::function<bool(size_t offset, DataSink &sink)>;

using ContentProviderResourceReleaser = std::function<void(bool success)>;

struct MultipartFormDataProvider {
  std::string name;
  ContentProviderWithoutLength provider;
  std::string filename;
  std::string content_type;
};
using MultipartFormDataProviderItems = std::vector<MultipartFormDataProvider>;

using ContentReceiverWithProgress =
    std::function<bool(const char *data, size_t data_length, uint64_t offset,
                       uint64_t total_length)>;

using ContentReceiver =
    std::function<bool(const char *data, size_t data_length)>;

using MultipartContentHeader =
    std::function<bool(const MultipartFormData &file)>;

class ContentReader {
public:
  using Reader = std::function<bool(ContentReceiver receiver)>;
  using MultipartReader = std::function<bool(MultipartContentHeader header,
                                             ContentReceiver receiver)>;

  ContentReader(Reader reader, MultipartReader multipart_reader)
      : reader_(std::move(reader)),
        multipart_reader_(std::move(multipart_reader)) {}

  bool operator()(MultipartContentHeader header,
                  ContentReceiver receiver) const {
    return multipart_reader_(std::move(header), std::move(receiver));
  }

  bool operator()(ContentReceiver receiver) const {
    return reader_(std::move(receiver));
  }

  Reader reader_;
  MultipartReader multipart_reader_;
};

using Range = std::pair<ssize_t, ssize_t>;
using Ranges = std::vector<Range>;

struct Request {
  std::string method;
  std::string path;
  Headers headers;
  std::string body;

  std::string remote_addr;
  int remote_port = -1;
  std::string local_addr;
  int local_port = -1;

  // for server
  std::string version;
  std::string target;
  Params params;
  MultipartFormDataMap files;
  Ranges ranges;
  Match matches;

  // for client
  ResponseHandler response_handler;
  ContentReceiverWithProgress content_receiver;
  Progress progress;
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
  const SSL *ssl = nullptr;
#endif

  bool has_header(const std::string &key) const;
  std::string get_header_value(const std::string &key, size_t id = 0) const;
  template <typename T>
  T get_header_value(const std::string &key, size_t id = 0) const;
  size_t get_header_value_count(const std::string &key) const;
  void set_header(const std::string &key, const std::string &val);

  bool has_param(const std::string &key) const;
  std::string get_param_value(const std::string &key, size_t id = 0) const;
  size_t get_param_value_count(const std::string &key) const;

  bool is_multipart_form_data() const;

  bool has_file(const std::string &key) const;
  MultipartFormData get_file_value(const std::string &key) const;
  std::vector<MultipartFormData> get_file_values(const std::string &key) const;

  // private members...
  size_t redirect_count_ = CPPHTTPLIB_REDIRECT_MAX_COUNT;
  size_t content_length_ = 0;
  ContentProvider content_provider_;
  bool is_chunked_content_provider_ = false;
  size_t authorization_count_ = 0;
};

struct Response {
  std::string version;
  int status = -1;
  std::string reason;
  Headers headers;
  std::string body;
  std::string location; // Redirect location

  bool has_header(const std::string &key) const;
  std::string get_header_value(const std::string &key, size_t id = 0) const;
  template <typename T>
  T get_header_value(const std::string &key, size_t id = 0) const;
  size_t get_header_value_count(const std::string &key) const;
  void set_header(const std::string &key, const std::string &val);

  void set_redirect(const std::string &url, int status = 302);
  void set_content(const char *s, size_t n, const std::string &content_type);
  void set_content(const std::string &s, const std::string &content_type);

  void set_content_provider(
      size_t length, const std::string &content_type, ContentProvider provider,
      ContentProviderResourceReleaser resource_releaser = nullptr);

  void set_content_provider(
      const std::string &content_type, ContentProviderWithoutLength provider,
      ContentProviderResourceReleaser resource_releaser = nullptr);

  void set_chunked_content_provider(
      const std::string &content_type, ContentProviderWithoutLength provider,
      ContentProviderResourceReleaser resource_releaser = nullptr);

  Response() = default;
  Response(const Response &) = default;
  Response &operator=(const Response &) = default;
  Response(Response &&) = default;
  Response &operator=(Response &&) = default;
  ~Response() {
    if (content_provider_resource_releaser_) {
      content_provider_resource_releaser_(content_provider_success_);
    }
  }

  // private members...
  size_t content_length_ = 0;
  ContentProvider content_provider_;
  ContentProviderResourceReleaser content_provider_resource_releaser_;
  bool is_chunked_content_provider_ = false;
  bool content_provider_success_ = false;
};

class Stream {
public:
  virtual ~Stream() = default;

  virtual bool is_readable() const = 0;
  virtual bool is_writable() const = 0;

  virtual ssize_t read(char *ptr, size_t size) = 0;
  virtual ssize_t write(const char *ptr, size_t size) = 0;
  virtual void get_remote_ip_and_port(std::string &ip, int &port) const = 0;
  virtual void get_local_ip_and_port(std::string &ip, int &port) const = 0;
  virtual socket_t socket() const = 0;

  template <typename... Args>
  ssize_t write_format(const char *fmt, const Args &...args);
  ssize_t write(const char *ptr);
  ssize_t write(const std::string &s);
};

class TaskQueue {
public:
  TaskQueue() = default;
  virtual ~TaskQueue() = default;

  virtual void enqueue(std::function<void()> fn) = 0;
  virtual void shutdown() = 0;

  virtual void on_idle() {}
};

class ThreadPool : public TaskQueue {
public:
  explicit ThreadPool(size_t n) : shutdown_(false) {
    while (n) {
      threads_.emplace_back(worker(*this));
      n--;
    }
  }

  ThreadPool(const ThreadPool &) = delete;
  ~ThreadPool() override = default;

  void enqueue(std::function<void()> fn) override {
    {
      std::unique_lock<std::mutex> lock(mutex_);
      jobs_.push_back(std::move(fn));
    }

    cond_.notify_one();
  }

  void shutdown() override {
    // Stop all worker threads...
    {
      std::unique_lock<std::mutex> lock(mutex_);
      shutdown_ = true;
    }

    cond_.notify_all();

    // Join...
    for (auto &t : threads_) {
      t.join();
    }
  }

private:
  struct worker {
    explicit worker(ThreadPool &pool) : pool_(pool) {}

    void operator()() {
      for (;;) {
        std::function<void()> fn;
        {
          std::unique_lock<std::mutex> lock(pool_.mutex_);

          pool_.cond_.wait(
              lock, [&] { return !pool_.jobs_.empty() || pool_.shutdown_; });

          if (pool_.shutdown_ && pool_.jobs_.empty()) { break; }

          fn = std::move(pool_.jobs_.front());
          pool_.jobs_.pop_front();
        }

        assert(true == static_cast<bool>(fn));
        fn();
      }
    }

    ThreadPool &pool_;
  };
  friend struct worker;

  std::vector<std::thread> threads_;
  std::list<std::function<void()>> jobs_;

  bool shutdown_;

  std::condition_variable cond_;
  std::mutex mutex_;
};

using Logger = std::function<void(const Request &, const Response &)>;

using SocketOptions = std::function<void(socket_t sock)>;

void default_socket_options(socket_t sock);

class Server {
public:
  using Handler = std::function<void(const Request &, Response &)>;

  using ExceptionHandler =
      std::function<void(const Request &, Response &, std::exception_ptr ep)>;

  enum class HandlerResponse {
    Handled,
    Unhandled,
  };
  using HandlerWithResponse =
      std::function<HandlerResponse(const Request &, Response &)>;

  using HandlerWithContentReader = std::function<void(
      const Request &, Response &, const ContentReader &content_reader)>;

  using Expect100ContinueHandler =
      std::function<int(const Request &, Response &)>;

  Server();

  virtual ~Server();

  virtual bool is_valid() const;

  Server &Get(const std::string &pattern, Handler handler);
  Server &Post(const std::string &pattern, Handler handler);
  Server &Post(const std::string &pattern, HandlerWithContentReader handler);
  Server &Put(const std::string &pattern, Handler handler);
  Server &Put(const std::string &pattern, HandlerWithContentReader handler);
  Server &Patch(const std::string &pattern, Handler handler);
  Server &Patch(const std::string &pattern, HandlerWithContentReader handler);
  Server &Delete(const std::string &pattern, Handler handler);
  Server &Delete(const std::string &pattern, HandlerWithContentReader handler);
  Server &Options(const std::string &pattern, Handler handler);

  bool set_base_dir(const std::string &dir,
                    const std::string &mount_point = std::string());
  bool set_mount_point(const std::string &mount_point, const std::string &dir,
                       Headers headers = Headers());
  bool remove_mount_point(const std::string &mount_point);
  Server &set_file_extension_and_mimetype_mapping(const std::string &ext,
                                                  const std::string &mime);
  Server &set_file_request_handler(Handler handler);

  Server &set_error_handler(HandlerWithResponse handler);
  Server &set_error_handler(Handler handler);
  Server &set_exception_handler(ExceptionHandler handler);
  Server &set_pre_routing_handler(HandlerWithResponse handler);
  Server &set_post_routing_handler(Handler handler);

  Server &set_expect_100_continue_handler(Expect100ContinueHandler handler);
  Server &set_logger(Logger logger);

  Server &set_address_family(int family);
  Server &set_tcp_nodelay(bool on);
  Server &set_socket_options(SocketOptions socket_options);

  Server &set_default_headers(Headers headers);

  Server &set_keep_alive_max_count(size_t count);
  Server &set_keep_alive_timeout(time_t sec);

  Server &set_read_timeout(time_t sec, time_t usec = 0);
  template <class Rep, class Period>
  Server &set_read_timeout(const std::chrono::duration<Rep, Period> &duration);

  Server &set_write_timeout(time_t sec, time_t usec = 0);
  template <class Rep, class Period>
  Server &set_write_timeout(const std::chrono::duration<Rep, Period> &duration);

  Server &set_idle_interval(time_t sec, time_t usec = 0);
  template <class Rep, class Period>
  Server &set_idle_interval(const std::chrono::duration<Rep, Period> &duration);

  Server &set_payload_max_length(size_t length);

  bool bind_to_port(const std::string &host, int port, int socket_flags = 0);
  int bind_to_any_port(const std::string &host, int socket_flags = 0);
  bool listen_after_bind();

  bool listen(const std::string &host, int port, int socket_flags = 0);

  bool is_running() const;
  void stop();

  std::function<TaskQueue *(void)> new_task_queue;

protected:
  bool process_request(Stream &strm, bool close_connection,
                       bool &connection_closed,
                       const std::function<void(Request &)> &setup_request);

  std::atomic<socket_t> svr_sock_;
  size_t keep_alive_max_count_ = CPPHTTPLIB_KEEPALIVE_MAX_COUNT;
  time_t keep_alive_timeout_sec_ = CPPHTTPLIB_KEEPALIVE_TIMEOUT_SECOND;
  time_t read_timeout_sec_ = CPPHTTPLIB_READ_TIMEOUT_SECOND;
  time_t read_timeout_usec_ = CPPHTTPLIB_READ_TIMEOUT_USECOND;
  time_t write_timeout_sec_ = CPPHTTPLIB_WRITE_TIMEOUT_SECOND;
  time_t write_timeout_usec_ = CPPHTTPLIB_WRITE_TIMEOUT_USECOND;
  time_t idle_interval_sec_ = CPPHTTPLIB_IDLE_INTERVAL_SECOND;
  time_t idle_interval_usec_ = CPPHTTPLIB_IDLE_INTERVAL_USECOND;
  size_t payload_max_length_ = CPPHTTPLIB_PAYLOAD_MAX_LENGTH;

private:
  using Handlers = std::vector<std::pair<std::regex, Handler>>;
  using HandlersForContentReader =
      std::vector<std::pair<std::regex, HandlerWithContentReader>>;

  socket_t create_server_socket(const std::string &host, int port,
                                int socket_flags,
                                SocketOptions socket_options) const;
  int bind_internal(const std::string &host, int port, int socket_flags);
  bool listen_internal();

  bool routing(Request &req, Response &res, Stream &strm);
  bool handle_file_request(const Request &req, Response &res,
                           bool head = false);
  bool dispatch_request(Request &req, Response &res, const Handlers &handlers);
  bool
  dispatch_request_for_content_reader(Request &req, Response &res,
                                      ContentReader content_reader,
                                      const HandlersForContentReader &handlers);

  bool parse_request_line(const char *s, Request &req);
  void apply_ranges(const Request &req, Response &res,
                    std::string &content_type, std::string &boundary);
  bool write_response(Stream &strm, bool close_connection, const Request &req,
                      Response &res);
  bool write_response_with_content(Stream &strm, bool close_connection,
                                   const Request &req, Response &res);
  bool write_response_core(Stream &strm, bool close_connection,
                           const Request &req, Response &res,
                           bool need_apply_ranges);
  bool write_content_with_provider(Stream &strm, const Request &req,
                                   Response &res, const std::string &boundary,
                                   const std::string &content_type);
  bool read_content(Stream &strm, Request &req, Response &res);
  bool
  read_content_with_content_receiver(Stream &strm, Request &req, Response &res,
                                     ContentReceiver receiver,
                                     MultipartContentHeader multipart_header,
                                     ContentReceiver multipart_receiver);
  bool read_content_core(Stream &strm, Request &req, Response &res,
                         ContentReceiver receiver,
                         MultipartContentHeader multipart_header,
                         ContentReceiver multipart_receiver);

  virtual bool process_and_close_socket(socket_t sock);

  struct MountPointEntry {
    std::string mount_point;
    std::string base_dir;
    Headers headers;
  };
  std::vector<MountPointEntry> base_dirs_;

  std::atomic<bool> is_running_;
  std::map<std::string, std::string> file_extension_and_mimetype_map_;
  Handler file_request_handler_;
  Handlers get_handlers_;
  Handlers post_handlers_;
  HandlersForContentReader post_handlers_for_content_reader_;
  Handlers put_handlers_;
  HandlersForContentReader put_handlers_for_content_reader_;
  Handlers patch_handlers_;
  HandlersForContentReader patch_handlers_for_content_reader_;
  Handlers delete_handlers_;
  HandlersForContentReader delete_handlers_for_content_reader_;
  Handlers options_handlers_;
  HandlerWithResponse error_handler_;
  ExceptionHandler exception_handler_;
  HandlerWithResponse pre_routing_handler_;
  Handler post_routing_handler_;
  Logger logger_;
  Expect100ContinueHandler expect_100_continue_handler_;

  int address_family_ = AF_UNSPEC;
  bool tcp_nodelay_ = CPPHTTPLIB_TCP_NODELAY;
  SocketOptions socket_options_ = default_socket_options;

  Headers default_headers_;
};

enum class Error {
  Success = 0,
  Unknown,
  Connection,
  BindIPAddress,
  Read,
  Write,
  ExceedRedirectCount,
  Canceled,
  SSLConnection,
  SSLLoadingCerts,
  SSLServerVerification,
  UnsupportedMultipartBoundaryChars,
  Compression,
  ConnectionTimeout,

  // For internal use only
  SSLPeerCouldBeClosed_,
};

std::string to_string(const Error error);

std::ostream &operator<<(std::ostream &os, const Error &obj);

class Result {
public:
  Result(std::unique_ptr<Response> &&res, Error err,
         Headers &&request_headers = Headers{})
      : res_(std::move(res)), err_(err),
        request_headers_(std::move(request_headers)) {}
  // Response
  operator bool() const { return res_ != nullptr; }
  bool operator==(std::nullptr_t) const { return res_ == nullptr; }
  bool operator!=(std::nullptr_t) const { return res_ != nullptr; }
  const Response &value() const { return *res_; }
  Response &value() { return *res_; }
  const Response &operator*() const { return *res_; }
  Response &operator*() { return *res_; }
  const Response *operator->() const { return res_.get(); }
  Response *operator->() { return res_.get(); }

  // Error
  Error error() const { return err_; }

  // Request Headers
  bool has_request_header(const std::string &key) const;
  std::string get_request_header_value(const std::string &key,
                                       size_t id = 0) const;
  template <typename T>
  T get_request_header_value(const std::string &key, size_t id = 0) const;
  size_t get_request_header_value_count(const std::string &key) const;

private:
  std::unique_ptr<Response> res_;
  Error err_;
  Headers request_headers_;
};

class ClientImpl {
public:
  explicit ClientImpl(const std::string &host);

  explicit ClientImpl(const std::string &host, int port);

  explicit ClientImpl(const std::string &host, int port,
                      const std::string &client_cert_path,
                      const std::string &client_key_path);

  virtual ~ClientImpl();

  virtual bool is_valid() const;

  Result Get(const std::string &path);
  Result Get(const std::string &path, const Headers &headers);
  Result Get(const std::string &path, Progress progress);
  Result Get(const std::string &path, const Headers &headers,
             Progress progress);
  Result Get(const std::string &path, ContentReceiver content_receiver);
  Result Get(const std::string &path, const Headers &headers,
             ContentReceiver content_receiver);
  Result Get(const std::string &path, ContentReceiver content_receiver,
             Progress progress);
  Result Get(const std::string &path, const Headers &headers,
             ContentReceiver content_receiver, Progress progress);
  Result Get(const std::string &path, ResponseHandler response_handler,
             ContentReceiver content_receiver);
  Result Get(const std::string &path, const Headers &headers,
             ResponseHandler response_handler,
             ContentReceiver content_receiver);
  Result Get(const std::string &path, ResponseHandler response_handler,
             ContentReceiver content_receiver, Progress progress);
  Result Get(const std::string &path, const Headers &headers,
             ResponseHandler response_handler, ContentReceiver content_receiver,
             Progress progress);

  Result Get(const std::string &path, const Params &params,
             const Headers &headers, Progress progress = nullptr);
  Result Get(const std::string &path, const Params &params,
             const Headers &headers, ContentReceiver content_receiver,
             Progress progress = nullptr);
  Result Get(const std::string &path, const Params &params,
             const Headers &headers, ResponseHandler response_handler,
             ContentReceiver content_receiver, Progress progress = nullptr);

  Result Head(const std::string &path);
  Result Head(const std::string &path, const Headers &headers);

  Result Post(const std::string &path);
  Result Post(const std::string &path, const Headers &headers);
  Result Post(const std::string &path, const char *body, size_t content_length,
              const std::string &content_type);
  Result Post(const std::string &path, const Headers &headers, const char *body,
              size_t content_length, const std::string &content_type);
  Result Post(const std::string &path, const std::string &body,
              const std::string &content_type);
  Result Post(const std::string &path, const Headers &headers,
              const std::string &body, const std::string &content_type);
  Result Post(const std::string &path, size_t content_length,
              ContentProvider content_provider,
              const std::string &content_type);
  Result Post(const std::string &path,
              ContentProviderWithoutLength content_provider,
              const std::string &content_type);
  Result Post(const std::string &path, const Headers &headers,
              size_t content_length, ContentProvider content_provider,
              const std::string &content_type);
  Result Post(const std::string &path, const Headers &headers,
              ContentProviderWithoutLength content_provider,
              const std::string &content_type);
  Result Post(const std::string &path, const Params &params);
  Result Post(const std::string &path, const Headers &headers,
              const Params &params);
  Result Post(const std::string &path, const MultipartFormDataItems &items);
  Result Post(const std::string &path, const Headers &headers,
              const MultipartFormDataItems &items);
  Result Post(const std::string &path, const Headers &headers,
              const MultipartFormDataItems &items, const std::string &boundary);
  Result Post(const std::string &path, const Headers &headers,
              const MultipartFormDataItems &items,
              const MultipartFormDataProviderItems &provider_items);

  Result Put(const std::string &path);
  Result Put(const std::string &path, const char *body, size_t content_length,
             const std::string &content_type);
  Result Put(const std::string &path, const Headers &headers, const char *body,
             size_t content_length, const std::string &content_type);
  Result Put(const std::string &path, const std::string &body,
             const std::string &content_type);
  Result Put(const std::string &path, const Headers &headers,
             const std::string &body, const std::string &content_type);
  Result Put(const std::string &path, size_t content_length,
             ContentProvider content_provider, const std::string &content_type);
  Result Put(const std::string &path,
             ContentProviderWithoutLength content_provider,
             const std::string &content_type);
  Result Put(const std::string &path, const Headers &headers,
             size_t content_length, ContentProvider content_provider,
             const std::string &content_type);
  Result Put(const std::string &path, const Headers &headers,
             ContentProviderWithoutLength content_provider,
             const std::string &content_type);
  Result Put(const std::string &path, const Params &params);
  Result Put(const std::string &path, const Headers &headers,
             const Params &params);
  Result Put(const std::string &path, const MultipartFormDataItems &items);
  Result Put(const std::string &path, const Headers &headers,
             const MultipartFormDataItems &items);
  Result Put(const std::string &path, const Headers &headers,
             const MultipartFormDataItems &items, const std::string &boundary);
  Result Put(const std::string &path, const Headers &headers,
             const MultipartFormDataItems &items,
             const MultipartFormDataProviderItems &provider_items);

  Result Patch(const std::string &path);
  Result Patch(const std::string &path, const char *body, size_t content_length,
               const std::string &content_type);
  Result Patch(const std::string &path, const Headers &headers,
               const char *body, size_t content_length,
               const std::string &content_type);
  Result Patch(const std::string &path, const std::string &body,
               const std::string &content_type);
  Result Patch(const std::string &path, const Headers &headers,
               const std::string &body, const std::string &content_type);
  Result Patch(const std::string &path, size_t content_length,
               ContentProvider content_provider,
               const std::string &content_type);
  Result Patch(const std::string &path,
               ContentProviderWithoutLength content_provider,
               const std::string &content_type);
  Result Patch(const std::string &path, const Headers &headers,
               size_t content_length, ContentProvider content_provider,
               const std::string &content_type);
  Result Patch(const std::string &path, const Headers &headers,
               ContentProviderWithoutLength content_provider,
               const std::string &content_type);

  Result Delete(const std::string &path);
  Result Delete(const std::string &path, const Headers &headers);
  Result Delete(const std::string &path, const char *body,
                size_t content_length, const std::string &content_type);
  Result Delete(const std::string &path, const Headers &headers,
                const char *body, size_t content_length,
                const std::string &content_type);
  Result Delete(const std::string &path, const std::string &body,
                const std::string &content_type);
  Result Delete(const std::string &path, const Headers &headers,
                const std::string &body, const std::string &content_type);

  Result Options(const std::string &path);
  Result Options(const std::string &path, const Headers &headers);

  bool send(Request &req, Response &res, Error &error);
  Result send(const Request &req);

  size_t is_socket_open() const;

  socket_t socket() const;

  void stop();

  void set_hostname_addr_map(std::map<std::string, std::string> addr_map);

  void set_default_headers(Headers headers);

  void set_address_family(int family);
  void set_tcp_nodelay(bool on);
  void set_socket_options(SocketOptions socket_options);

  void set_connection_timeout(time_t sec, time_t usec = 0);
  template <class Rep, class Period>
  void
  set_connection_timeout(const std::chrono::duration<Rep, Period> &duration);

  void set_read_timeout(time_t sec, time_t usec = 0);
  template <class Rep, class Period>
  void set_read_timeout(const std::chrono::duration<Rep, Period> &duration);

  void set_write_timeout(time_t sec, time_t usec = 0);
  template <class Rep, class Period>
  void set_write_timeout(const std::chrono::duration<Rep, Period> &duration);

  void set_basic_auth(const std::string &username, const std::string &password);
  void set_bearer_token_auth(const std::string &token);
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
  void set_digest_auth(const std::string &username,
                       const std::string &password);
#endif

  void set_keep_alive(bool on);
  void set_follow_location(bool on);

  void set_url_encode(bool on);

  void set_compress(bool on);

  void set_decompress(bool on);

  void set_interface(const std::string &intf);

  void set_proxy(const std::string &host, int port);
  void set_proxy_basic_auth(const std::string &username,
                            const std::string &password);
  void set_proxy_bearer_token_auth(const std::string &token);
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
  void set_proxy_digest_auth(const std::string &username,
                             const std::string &password);
#endif

#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
  void set_ca_cert_path(const std::string &ca_cert_file_path,
                        const std::string &ca_cert_dir_path = std::string());
  void set_ca_cert_store(X509_STORE *ca_cert_store);
#endif

#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
  void enable_server_certificate_verification(bool enabled);
#endif

  void set_logger(Logger logger);

protected:
  struct Socket {
    socket_t sock = INVALID_SOCKET;
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
    SSL *ssl = nullptr;
#endif

    bool is_open() const { return sock != INVALID_SOCKET; }
  };

  virtual bool create_and_connect_socket(Socket &socket, Error &error);

  // All of:
  //   shutdown_ssl
  //   shutdown_socket
  //   close_socket
  // should ONLY be called when socket_mutex_ is locked.
  // Also, shutdown_ssl and close_socket should also NOT be called concurrently
  // with a DIFFERENT thread sending requests using that socket.
  virtual void shutdown_ssl(Socket &socket, bool shutdown_gracefully);
  void shutdown_socket(Socket &socket);
  void close_socket(Socket &socket);

  bool process_request(Stream &strm, Request &req, Response &res,
                       bool close_connection, Error &error);

  bool write_content_with_provider(Stream &strm, const Request &req,
                                   Error &error);

  void copy_settings(const ClientImpl &rhs);

  // Socket endpoint information
  const std::string host_;
  const int port_;
  const std::string host_and_port_;

  // Current open socket
  Socket socket_;
  mutable std::mutex socket_mutex_;
  std::recursive_mutex request_mutex_;

  // These are all protected under socket_mutex
  size_t socket_requests_in_flight_ = 0;
  std::thread::id socket_requests_are_from_thread_ = std::thread::id();
  bool socket_should_be_closed_when_request_is_done_ = false;

  // Hostname-IP map
  std::map<std::string, std::string> addr_map_;

  // Default headers
  Headers default_headers_;

  // Settings
  std::string client_cert_path_;
  std::string client_key_path_;

  time_t connection_timeout_sec_ = CPPHTTPLIB_CONNECTION_TIMEOUT_SECOND;
  time_t connection_timeout_usec_ = CPPHTTPLIB_CONNECTION_TIMEOUT_USECOND;
  time_t read_timeout_sec_ = CPPHTTPLIB_READ_TIMEOUT_SECOND;
  time_t read_timeout_usec_ = CPPHTTPLIB_READ_TIMEOUT_USECOND;
  time_t write_timeout_sec_ = CPPHTTPLIB_WRITE_TIMEOUT_SECOND;
  time_t write_timeout_usec_ = CPPHTTPLIB_WRITE_TIMEOUT_USECOND;

  std::string basic_auth_username_;
  std::string basic_auth_password_;
  std::string bearer_token_auth_token_;
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
  std::string digest_auth_username_;
  std::string digest_auth_password_;
#endif

  bool keep_alive_ = false;
  bool follow_location_ = false;

  bool url_encode_ = true;

  int address_family_ = AF_UNSPEC;
  bool tcp_nodelay_ = CPPHTTPLIB_TCP_NODELAY;
  SocketOptions socket_options_ = nullptr;

  bool compress_ = false;
  bool decompress_ = true;

  std::string interface_;

  std::string proxy_host_;
  int proxy_port_ = -1;

  std::string proxy_basic_auth_username_;
  std::string proxy_basic_auth_password_;
  std::string proxy_bearer_token_auth_token_;
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
  std::string proxy_digest_auth_username_;
  std::string proxy_digest_auth_password_;
#endif

#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
  std::string ca_cert_file_path_;
  std::string ca_cert_dir_path_;

  X509_STORE *ca_cert_store_ = nullptr;
#endif

#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
  bool server_certificate_verification_ = true;
#endif

  Logger logger_;

private:
  bool send_(Request &req, Response &res, Error &error);
  Result send_(Request &&req);

  socket_t create_client_socket(Error &error) const;
  bool read_response_line(Stream &strm, const Request &req, Response &res);
  bool write_request(Stream &strm, Request &req, bool close_connection,
                     Error &error);
  bool redirect(Request &req, Response &res, Error &error);
  bool handle_request(Stream &strm, Request &req, Response &res,
                      bool close_connection, Error &error);
  std::unique_ptr<Response> send_with_content_provider(
      Request &req, const char *body, size_t content_length,
      ContentProvider content_provider,
      ContentProviderWithoutLength content_provider_without_length,
      const std::string &content_type, Error &error);
  Result send_with_content_provider(
      const std::string &method, const std::string &path,
      const Headers &headers, const char *body, size_t content_length,
      ContentProvider content_provider,
      ContentProviderWithoutLength content_provider_without_length,
      const std::string &content_type);
  ContentProviderWithoutLength get_multipart_content_provider(
      const std::string &boundary, const MultipartFormDataItems &items,
      const MultipartFormDataProviderItems &provider_items);

  std::string adjust_host_string(const std::string &host) const;

  virtual bool process_socket(const Socket &socket,
                              std::function<bool(Stream &strm)> callback);
  virtual bool is_ssl() const;
};

class Client {
public:
  // Universal interface
  explicit Client(const std::string &scheme_host_port);

  explicit Client(const std::string &scheme_host_port,
                  const std::string &client_cert_path,
                  const std::string &client_key_path);

  // HTTP only interface
  explicit Client(const std::string &host, int port);

  explicit Client(const std::string &host, int port,
                  const std::string &client_cert_path,
                  const std::string &client_key_path);

  Client(Client &&) = default;

  ~Client();

  bool is_valid() const;

  Result Get(const std::string &path);
  Result Get(const std::string &path, const Headers &headers);
  Result Get(const std::string &path, Progress progress);
  Result Get(const std::string &path, const Headers &headers,
             Progress progress);
  Result Get(const std::string &path, ContentReceiver content_receiver);
  Result Get(const std::string &path, const Headers &headers,
             ContentReceiver content_receiver);
  Result Get(const std::string &path, ContentReceiver content_receiver,
             Progress progress);
  Result Get(const std::string &path, const Headers &headers,
             ContentReceiver content_receiver, Progress progress);
  Result Get(const std::string &path, ResponseHandler response_handler,
             ContentReceiver content_receiver);
  Result Get(const std::string &path, const Headers &headers,
             ResponseHandler response_handler,
             ContentReceiver content_receiver);
  Result Get(const std::string &path, const Headers &headers,
             ResponseHandler response_handler, ContentReceiver content_receiver,
             Progress progress);
  Result Get(const std::string &path, ResponseHandler response_handler,
             ContentReceiver content_receiver, Progress progress);

  Result Get(const std::string &path, const Params &params,
             const Headers &headers, Progress progress = nullptr);
  Result Get(const std::string &path, const Params &params,
             const Headers &headers, ContentReceiver content_receiver,
             Progress progress = nullptr);
  Result Get(const std::string &path, const Params &params,
             const Headers &headers, ResponseHandler response_handler,
             ContentReceiver content_receiver, Progress progress = nullptr);

  Result Head(const std::string &path);
  Result Head(const std::string &path, const Headers &headers);

  Result Post(const std::string &path);
  Result Post(const std::string &path, const Headers &headers);
  Result Post(const std::string &path, const char *body, size_t content_length,
              const std::string &content_type);
  Result Post(const std::string &path, const Headers &headers, const char *body,
              size_t content_length, const std::string &content_type);
  Result Post(const std::string &path, const std::string &body,
              const std::string &content_type);
  Result Post(const std::string &path, const Headers &headers,
              const std::string &body, const std::string &content_type);
  Result Post(const std::string &path, size_t content_length,
              ContentProvider content_provider,
              const std::string &content_type);
  Result Post(const std::string &path,
              ContentProviderWithoutLength content_provider,
              const std::string &content_type);
  Result Post(const std::string &path, const Headers &headers,
              size_t content_length, ContentProvider content_provider,
              const std::string &content_type);
  Result Post(const std::string &path, const Headers &headers,
              ContentProviderWithoutLength content_provider,
              const std::string &content_type);
  Result Post(const std::string &path, const Params &params);
  Result Post(const std::string &path, const Headers &headers,
              const Params &params);
  Result Post(const std::string &path, const MultipartFormDataItems &items);
  Result Post(const std::string &path, const Headers &headers,
              const MultipartFormDataItems &items);
  Result Post(const std::string &path, const Headers &headers,
              const MultipartFormDataItems &items, const std::string &boundary);
  Result Post(const std::string &path, const Headers &headers,
              const MultipartFormDataItems &items,
              const MultipartFormDataProviderItems &provider_items);

  Result Put(const std::string &path);
  Result Put(const std::string &path, const char *body, size_t content_length,
             const std::string &content_type);
  Result Put(const std::string &path, const Headers &headers, const char *body,
             size_t content_length, const std::string &content_type);
  Result Put(const std::string &path, const std::string &body,
             const std::string &content_type);
  Result Put(const std::string &path, const Headers &headers,
             const std::string &body, const std::string &content_type);
  Result Put(const std::string &path, size_t content_length,
             ContentProvider content_provider, const std::string &content_type);
  Result Put(const std::string &path,
             ContentProviderWithoutLength content_provider,
             const std::string &content_type);
  Result Put(const std::string &path, const Headers &headers,
             size_t content_length, ContentProvider content_provider,
             const std::string &content_type);
  Result Put(const std::string &path, const Headers &headers,
             ContentProviderWithoutLength content_provider,
             const std::string &content_type);
  Result Put(const std::string &path, const Params &params);
  Result Put(const std::string &path, const Headers &headers,
             const Params &params);
  Result Put(const std::string &path, const MultipartFormDataItems &items);
  Result Put(const std::string &path, const Headers &headers,
             const MultipartFormDataItems &items);
  Result Put(const std::string &path, const Headers &headers,
             const MultipartFormDataItems &items, const std::string &boundary);
  Result Put(const std::string &path, const Headers &headers,
             const MultipartFormDataItems &items,
             const MultipartFormDataProviderItems &provider_items);

  Result Patch(const std::string &path);
  Result Patch(const std::string &path, const char *body, size_t content_length,
               const std::string &content_type);
  Result Patch(const std::string &path, const Headers &headers,
               const char *body, size_t content_length,
               const std::string &content_type);
  Result Patch(const std::string &path, const std::string &body,
               const std::string &content_type);
  Result Patch(const std::string &path, const Headers &headers,
               const std::string &body, const std::string &content_type);
  Result Patch(const std::string &path, size_t content_length,
               ContentProvider content_provider,
               const std::string &content_type);
  Result Patch(const std::string &path,
               ContentProviderWithoutLength content_provider,
               const std::string &content_type);
  Result Patch(const std::string &path, const Headers &headers,
               size_t content_length, ContentProvider content_provider,
               const std::string &content_type);
  Result Patch(const std::string &path, const Headers &headers,
               ContentProviderWithoutLength content_provider,
               const std::string &content_type);

  Result Delete(const std::string &path);
  Result Delete(const std::string &path, const Headers &headers);
  Result Delete(const std::string &path, const char *body,
                size_t content_length, const std::string &content_type);
  Result Delete(const std::string &path, const Headers &headers,
                const char *body, size_t content_length,
                const std::string &content_type);
  Result Delete(const std::string &path, const std::string &body,
                const std::string &content_type);
  Result Delete(const std::string &path, const Headers &headers,
                const std::string &body, const std::string &content_type);

  Result Options(const std::string &path);
  Result Options(const std::string &path, const Headers &headers);

  bool send(Request &req, Response &res, Error &error);
  Result send(const Request &req);

  size_t is_socket_open() const;

  socket_t socket() const;

  void stop();

  void set_hostname_addr_map(std::map<std::string, std::string> addr_map);

  void set_default_headers(Headers headers);

  void set_address_family(int family);
  void set_tcp_nodelay(bool on);
  void set_socket_options(SocketOptions socket_options);

  void set_connection_timeout(time_t sec, time_t usec = 0);
  template <class Rep, class Period>
  void
  set_connection_timeout(const std::chrono::duration<Rep, Period> &duration);

  void set_read_timeout(time_t sec, time_t usec = 0);
  template <class Rep, class Period>
  void set_read_timeout(const std::chrono::duration<Rep, Period> &duration);

  void set_write_timeout(time_t sec, time_t usec = 0);
  template <class Rep, class Period>
  void set_write_timeout(const std::chrono::duration<Rep, Period> &duration);

  void set_basic_auth(const std::string &username, const std::string &password);
  void set_bearer_token_auth(const std::string &token);
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
  void set_digest_auth(const std::string &username,
                       const std::string &password);
#endif

  void set_keep_alive(bool on);
  void set_follow_location(bool on);

  void set_url_encode(bool on);

  void set_compress(bool on);

  void set_decompress(bool on);

  void set_interface(const std::string &intf);

  void set_proxy(const std::string &host, int port);
  void set_proxy_basic_auth(const std::string &username,
                            const std::string &password);
  void set_proxy_bearer_token_auth(const std::string &token);
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
  void set_proxy_digest_auth(const std::string &username,
                             const std::string &password);
#endif

#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
  void enable_server_certificate_verification(bool enabled);
#endif

  void set_logger(Logger logger);

  // SSL
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
  void set_ca_cert_path(const std::string &ca_cert_file_path,
                        const std::string &ca_cert_dir_path = std::string());

  void set_ca_cert_store(X509_STORE *ca_cert_store);

  long get_openssl_verify_result() const;

  SSL_CTX *ssl_context() const;
#endif

private:
  std::unique_ptr<ClientImpl> cli_;

#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
  bool is_ssl_ = false;
#endif
};

#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
class SSLServer : public Server {
public:
  SSLServer(const char *cert_path, const char *private_key_path,
            const char *client_ca_cert_file_path = nullptr,
            const char *client_ca_cert_dir_path = nullptr,
            const char *private_key_password = nullptr);

  SSLServer(X509 *cert, EVP_PKEY *private_key,
            X509_STORE *client_ca_cert_store = nullptr);

  SSLServer(
      const std::function<bool(SSL_CTX &ssl_ctx)> &setup_ssl_ctx_callback);

  ~SSLServer() override;

  bool is_valid() const override;

  SSL_CTX *ssl_context() const;

private:
  bool process_and_close_socket(socket_t sock) override;

  SSL_CTX *ctx_;
  std::mutex ctx_mutex_;
};

class SSLClient : public ClientImpl {
public:
  explicit SSLClient(const std::string &host);

  explicit SSLClient(const std::string &host, int port);

  explicit SSLClient(const std::string &host, int port,
                     const std::string &client_cert_path,
                     const std::string &client_key_path);

  explicit SSLClient(const std::string &host, int port, X509 *client_cert,
                     EVP_PKEY *client_key);

  ~SSLClient() override;

  bool is_valid() const override;

  void set_ca_cert_store(X509_STORE *ca_cert_store);

  long get_openssl_verify_result() const;

  SSL_CTX *ssl_context() const;

private:
  bool create_and_connect_socket(Socket &socket, Error &error) override;
  void shutdown_ssl(Socket &socket, bool shutdown_gracefully) override;
  void shutdown_ssl_impl(Socket &socket, bool shutdown_socket);

  bool process_socket(const Socket &socket,
                      std::function<bool(Stream &strm)> callback) override;
  bool is_ssl() const override;

  bool connect_with_proxy(Socket &sock, Response &res, bool &success,
                          Error &error);
  bool initialize_ssl(Socket &socket, Error &error);

  bool load_certs();

  bool verify_host(X509 *server_cert) const;
  bool verify_host_with_subject_alt_name(X509 *server_cert) const;
  bool verify_host_with_common_name(X509 *server_cert) const;
  bool check_host_name(const char *pattern, size_t pattern_len) const;

  SSL_CTX *ctx_;
  std::mutex ctx_mutex_;
  std::once_flag initialize_cert_;

  std::vector<std::string> host_components_;

  long verify_result_ = 0;

  friend class ClientImpl;
};
#endif

/*
 * Implementation of template methods.
 */

namespace detail {

template <typename T, typename U>
inline void duration_to_sec_and_usec(const T &duration, U callback) {
  auto sec = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
  auto usec = std::chrono::duration_cast<std::chrono::microseconds>(
                  duration - std::chrono::seconds(sec))
                  .count();
  callback(static_cast<time_t>(sec), static_cast<time_t>(usec));
}

template <typename T>
inline T get_header_value(const Headers & /*headers*/,
                          const std::string & /*key*/, size_t /*id*/ = 0,
                          uint64_t /*def*/ = 0) {}

template <>
inline uint64_t get_header_value<uint64_t>(const Headers &headers,
                                           const std::string &key, size_t id,
                                           uint64_t def) {
  auto rng = headers.equal_range(key);
  auto it = rng.first;
  std::advance(it, static_cast<ssize_t>(id));
  if (it != rng.second) {
    return std::strtoull(it->second.data(), nullptr, 10);
  }
  return def;
}

} // namespace detail

template <typename T>
inline T Request::get_header_value(const std::string &key, size_t id) const {
  return detail::get_header_value<T>(headers, key, id, 0);
}

template <typename T>
inline T Response::get_header_value(const std::string &key, size_t id) const {
  return detail::get_header_value<T>(headers, key, id, 0);
}

template <typename... Args>
inline ssize_t Stream::write_format(const char *fmt, const Args &...args) {
  const auto bufsiz = 2048;
  std::array<char, bufsiz> buf{};

  auto sn = snprintf(buf.data(), buf.size() - 1, fmt, args...);
  if (sn <= 0) { return sn; }

  auto n = static_cast<size_t>(sn);

  if (n >= buf.size() - 1) {
    std::vector<char> glowable_buf(buf.size());

    while (n >= glowable_buf.size() - 1) {
      glowable_buf.resize(glowable_buf.size() * 2);
      n = static_cast<size_t>(
          snprintf(&glowable_buf[0], glowable_buf.size() - 1, fmt, args...));
    }
    return write(&glowable_buf[0], n);
  } else {
    return write(buf.data(), n);
  }
}

inline void default_socket_options(socket_t sock) {
  int yes = 1;
#ifdef _WIN32
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<char *>(&yes),
             sizeof(yes));
  setsockopt(sock, SOL_SOCKET, SO_EXCLUSIVEADDRUSE,
             reinterpret_cast<char *>(&yes), sizeof(yes));
#else
#ifdef SO_REUSEPORT
  setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, reinterpret_cast<void *>(&yes),
             sizeof(yes));
#else
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<void *>(&yes),
             sizeof(yes));
#endif
#endif
}

template <class Rep, class Period>
inline Server &
Server::set_read_timeout(const std::chrono::duration<Rep, Period> &duration) {
  detail::duration_to_sec_and_usec(
      duration, [&](time_t sec, time_t usec) { set_read_timeout(sec, usec); });
  return *this;
}

template <class Rep, class Period>
inline Server &
Server::set_write_timeout(const std::chrono::duration<Rep, Period> &duration) {
  detail::duration_to_sec_and_usec(
      duration, [&](time_t sec, time_t usec) { set_write_timeout(sec, usec); });
  return *this;
}

template <class Rep, class Period>
inline Server &
Server::set_idle_interval(const std::chrono::duration<Rep, Period> &duration) {
  detail::duration_to_sec_and_usec(
      duration, [&](time_t sec, time_t usec) { set_idle_interval(sec, usec); });
  return *this;
}

inline std::string to_string(const Error error) {
  switch (error) {
  case Error::Success: return "Success (no error)";
  case Error::Connection: return "Could not establish connection";
  case Error::BindIPAddress: return "Failed to bind IP address";
  case Error::Read: return "Failed to read connection";
  case Error::Write: return "Failed to write connection";
  case Error::ExceedRedirectCount: return "Maximum redirect count exceeded";
  case Error::Canceled: return "Connection handling canceled";
  case Error::SSLConnection: return "SSL connection failed";
  case Error::SSLLoadingCerts: return "SSL certificate loading failed";
  case Error::SSLServerVerification: return "SSL server verification failed";
  case Error::UnsupportedMultipartBoundaryChars:
    return "Unsupported HTTP multipart boundary characters";
  case Error::Compression: return "Compression failed";
  case Error::ConnectionTimeout: return "Connection timed out";
  case Error::Unknown: return "Unknown";
  default: break;
  }

  return "Invalid";
}

inline std::ostream &operator<<(std::ostream &os, const Error &obj) {
  os << to_string(obj);
  os << " (" << static_cast<std::underlying_type<Error>::type>(obj) << ')';
  return os;
}

template <typename T>
inline T Result::get_request_header_value(const std::string &key,
                                          size_t id) const {
  return detail::get_header_value<T>(request_headers_, key, id, 0);
}

template <class Rep, class Period>
inline void ClientImpl::set_connection_timeout(
    const std::chrono::duration<Rep, Period> &duration) {
  detail::duration_to_sec_and_usec(duration, [&](time_t sec, time_t usec) {
    set_connection_timeout(sec, usec);
  });
}

template <class Rep, class Period>
inline void ClientImpl::set_read_timeout(
    const std::chrono::duration<Rep, Period> &duration) {
  detail::duration_to_sec_and_usec(
      duration, [&](time_t sec, time_t usec) { set_read_timeout(sec, usec); });
}

template <class Rep, class Period>
inline void ClientImpl::set_write_timeout(
    const std::chrono::duration<Rep, Period> &duration) {
  detail::duration_to_sec_and_usec(
      duration, [&](time_t sec, time_t usec) { set_write_timeout(sec, usec); });
}

template <class Rep, class Period>
inline void Client::set_connection_timeout(
    const std::chrono::duration<Rep, Period> &duration) {
  cli_->set_connection_timeout(duration);
}

template <class Rep, class Period>
inline void
Client::set_read_timeout(const std::chrono::duration<Rep, Period> &duration) {
  cli_->set_read_timeout(duration);
}

template <class Rep, class Period>
inline void
Client::set_write_timeout(const std::chrono::duration<Rep, Period> &duration) {
  cli_->set_write_timeout(duration);
}

/*
 * Forward declarations and types that will be part of the .h file if split into
 * .h + .cc.
 */

std::string hosted_at(const std::string &hostname);

void hosted_at(const std::string &hostname, std::vector<std::string> &addrs);

std::string append_query_params(const std::string &path, const Params &params);

std::pair<std::string, std::string> make_range_header(Ranges ranges);

std::pair<std::string, std::string>
make_basic_authentication_header(const std::string &username,
                                 const std::string &password,
                                 bool is_proxy = false);

namespace detail {

std::string encode_query_param(const std::string &value);

std::string decode_url(const std::string &s, bool convert_plus_to_space);

void read_file(const std::string &path, std::string &out);

std::string trim_copy(const std::string &s);

void split(const char *b, const char *e, char d,
           std::function<void(const char *, const char *)> fn);

bool process_client_socket(socket_t sock, time_t read_timeout_sec,
                           time_t read_timeout_usec, time_t write_timeout_sec,
                           time_t write_timeout_usec,
                           std::function<bool(Stream &)> callback);

socket_t create_client_socket(
    const std::string &host, const std::string &ip, int port,
    int address_family, bool tcp_nodelay, SocketOptions socket_options,
    time_t connection_timeout_sec, time_t connection_timeout_usec,
    time_t read_timeout_sec, time_t read_timeout_usec, time_t write_timeout_sec,
    time_t write_timeout_usec, const std::string &intf, Error &error);

const char *get_header_value(const Headers &headers, const std::string &key,
                             size_t id = 0, const char *def = nullptr);

std::string params_to_query_str(const Params &params);

void parse_query_text(const std::string &s, Params &params);

bool parse_multipart_boundary(const std::string &content_type,
                              std::string &boundary);

bool parse_range_header(const std::string &s, Ranges &ranges);

int close_socket(socket_t sock);

ssize_t send_socket(socket_t sock, const void *ptr, size_t size, int flags);

ssize_t read_socket(socket_t sock, void *ptr, size_t size, int flags);

enum class EncodingType { None = 0, Gzip, Brotli };

EncodingType encoding_type(const Request &req, const Response &res);

class BufferStream : public Stream {
public:
  BufferStream() = default;
  ~BufferStream() override = default;

  bool is_readable() const override;
  bool is_writable() const override;
  ssize_t read(char *ptr, size_t size) override;
  ssize_t write(const char *ptr, size_t size) override;
  void get_remote_ip_and_port(std::string &ip, int &port) const override;
  void get_local_ip_and_port(std::string &ip, int &port) const override;
  socket_t socket() const override;

  const std::string &get_buffer() const;

private:
  std::string buffer;
  size_t position = 0;
};

class compressor {
public:
  virtual ~compressor() = default;

  typedef std::function<bool(const char *data, size_t data_len)> Callback;
  virtual bool compress(const char *data, size_t data_length, bool last,
                        Callback callback) = 0;
};

class decompressor {
public:
  virtual ~decompressor() = default;

  virtual bool is_valid() const = 0;

  typedef std::function<bool(const char *data, size_t data_len)> Callback;
  virtual bool decompress(const char *data, size_t data_length,
                          Callback callback) = 0;
};

class nocompressor : public compressor {
public:
  virtual ~nocompressor() = default;

  bool compress(const char *data, size_t data_length, bool /*last*/,
                Callback callback) override;
};

#ifdef CPPHTTPLIB_ZLIB_SUPPORT
class gzip_compressor : public compressor {
public:
  gzip_compressor();
  ~gzip_compressor();

  bool compress(const char *data, size_t data_length, bool last,
                Callback callback) override;

private:
  bool is_valid_ = false;
  z_stream strm_;
};

class gzip_decompressor : public decompressor {
public:
  gzip_decompressor();
  ~gzip_decompressor();

  bool is_valid() const override;

  bool decompress(const char *data, size_t data_length,
                  Callback callback) override;

private:
  bool is_valid_ = false;
  z_stream strm_;
};
#endif

#ifdef CPPHTTPLIB_BROTLI_SUPPORT
class brotli_compressor : public compressor {
public:
  brotli_compressor();
  ~brotli_compressor();

  bool compress(const char *data, size_t data_length, bool last,
                Callback callback) override;

private:
  BrotliEncoderState *state_ = nullptr;
};

class brotli_decompressor : public decompressor {
public:
  brotli_decompressor();
  ~brotli_decompressor();

  bool is_valid() const override;

  bool decompress(const char *data, size_t data_length,
                  Callback callback) override;

private:
  BrotliDecoderResult decoder_r;
  BrotliDecoderState *decoder_s = nullptr;
};
#endif

// NOTE: until the read size reaches `fixed_buffer_size`, use `fixed_buffer`
// to store data. The call can set memory on stack for performance.
class stream_line_reader {
public:
  stream_line_reader(Stream &strm, char *fixed_buffer,
                     size_t fixed_buffer_size);
  const char *ptr() const;
  size_t size() const;
  bool end_with_crlf() const;
  bool getline();

private:
  void append(char c);

  Stream &strm_;
  char *fixed_buffer_;
  const size_t fixed_buffer_size_;
  size_t fixed_buffer_used_size_ = 0;
  std::string glowable_buffer_;
};

} // namespace detail


} // namespace httplib

#if defined(_WIN32) && defined(CPPHTTPLIB_USE_POLL)
#undef poll
#endif

#endif // CPPHTTPLIB_HTTPLIB_H

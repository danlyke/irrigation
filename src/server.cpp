#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <microhttpd.h>
#include <time.h>
#include <string>

using namespace std;

#define PORT 8888

#define VALVE_COUNT 8

time_t valve_off_times[VALVE_COUNT];
const char *valve_names[VALVE_COUNT] =
{
    "Front Yard",
    "East Side",
    "Back Close and West Side",
    "Lemon Tree",
    "Beans &amp; Amaranth",
    "Berries",
    "Lawn",
    "unknown1",
};


int answer_to_connection (void *cls, struct MHD_Connection *connection, 
                          const char *url, 
                          const char *method, const char *version, 
                          const char *upload_data, 
                          size_t *upload_data_size, void **con_cls)
{
    time_t now = time(NULL);

    string page("<html><head><title>Irrigation</title></head><body><h1>Irrigation</h1>\n");
    page += "<table><tr><th>Timer</th><th>Minutes</th><th>Start</th></tr>\n";

    for (int valve_num = 0; valve_num < VALVE_COUNT; ++valve_num)
    {
        page += "<tr><td>";
        page += valve_names[valve_num];
        page += "</td><td id=\"valve_time_" + to_string(valve_num) + "\">";
        if (valve_off_times[valve_num] < now)
        {
            page += "off";
            page += "</td><td><a href=\"/start/" + to_string(valve_num) + "\">Start</a></td>";
        }
        else
        {
            char time_buf[32];
            time_t time_remaining = valve_off_times[valve_num] - now;
            snprintf(time_buf, sizeof(time_buf), "%ld:%.2ld",
                     time_remaining / 60,
                     time_remaining % 60);
            page += time_buf;
            page += "off";
            page += "</td><td><a href=\"/stop/" + to_string(valve_num) + "\">Stop</a></td>";
        }
        page += "</tr>\n";
    }
    page += "</table>";


    page += "</body></html>\n";
    struct MHD_Response *response;
    int ret;

    response = MHD_create_response_from_buffer (page.length(),
                                                const_cast< void*>((void *)page.data()), MHD_RESPMEM_MUST_COPY);
    ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
    MHD_destroy_response (response);

    fprintf(stderr, "Responded to URL %s method %s\n", url, method);
    return ret;
}

#define KEEPALIVE_TIME 10

int main ()
{
  struct MHD_Daemon *daemon;

  daemon = MHD_start_daemon (MHD_USE_DEBUG, PORT, NULL, NULL, 
                             &answer_to_connection, NULL, MHD_OPTION_END);
  if (NULL == daemon) return 1;

  time_t previous_time(time(NULL));

  while (1)
  {
      fd_set              read_fd_set, write_fd_set, except_fd_set;
      FD_ZERO(&read_fd_set);
      FD_ZERO(&write_fd_set);
      FD_ZERO(&except_fd_set);
      int max_fd(0);
      struct timeval      tv;
      memset(&tv,0,sizeof(tv));
      tv.tv_sec  = KEEPALIVE_TIME;
      tv.tv_usec = 0;

      MHD_get_fdset(daemon, &read_fd_set, &write_fd_set, &except_fd_set, &max_fd);
      
      if (-1 != select(max_fd+1,
                       &read_fd_set,
                       &write_fd_set,
                       &except_fd_set,
                       &tv))
      {
          MHD_run_from_select(daemon, &read_fd_set, &write_fd_set, &except_fd_set);
      }
      time_t now;
      now = time(NULL);
      printf("%ld seconds elapsed\n", now - previous_time);
      previous_time = now;
  }

  MHD_stop_daemon (daemon);
  return 0;
}

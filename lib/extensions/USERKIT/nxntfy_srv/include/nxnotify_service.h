#ifndef NEXSOS_NXNOTIFY_SERVICE_H
#define NEXSOS_NXNOTIFY_SERVICE_H

#define OS1NX_PORT_NOTIFY "OS1nx_svc_notify"

int OS1_notify_post(const char *title, const char *message);
int OS1_notify_warn(const char *title, const char *message);
int OS1_notify_error(const char *title, const char *message);
int notify(const char *title, const char *message);

#endif /* NEXSOS_NXNOTIFY_SERVICE_H */

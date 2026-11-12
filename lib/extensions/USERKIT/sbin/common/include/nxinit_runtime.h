#ifndef NEXSOS_USERKIT_NXINIT_RUNTIME_H
#define NEXSOS_USERKIT_NXINIT_RUNTIME_H

struct nxinit_identity {
  const char *name;
  const char *boot_message;
};

int nxinit_run(const struct nxinit_identity *identity);

#endif

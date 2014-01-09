#ifndef _CRYPT_H_
#define _CRYPT_H_

  // FIX: should this be null-terminated or not?
  extern "C" void encrypt(char @{64}, int);

  extern "C" char * crypt(const char @ key, const char @ salt);
#endif

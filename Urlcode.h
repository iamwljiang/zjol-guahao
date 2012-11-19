#ifndef GUAHAO_URLDECODE_H_
#define GUAHAO_URLDECODE_H_
//just decode url or encode url,not to do character decode/encode
//copy from sinae sae open code
#ifdef __cplusplus
extern "C" {
#endif

int url_encode(char** pout, const char* pin ,  int len);

int url_decode(char** pout,const char* pin , int len);

#ifdef __cplusplus
}
#endif

#endif //GUAHAO_URLDECODE_H_
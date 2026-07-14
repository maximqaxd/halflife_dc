#ifndef KZAP_H
#define KZAP_H
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void KZapInitCodecs( int maxarg );
int  KZapCompress( byte *src, byte *dst, int len );
int  KZapDecompress( byte *src, byte *dst );
int  ZlibCompress( byte *src, byte *dst, int len );
int  ZlibDecompress( byte *src, byte *dst );

#ifdef __cplusplus
}
#endif

#endif // KZAP_H

# why and how to compile/install liboauth-0.9.5

# Introduction #

This is a guide document for why and how to compile liboauth-0.9.5.

# Why do we need to use liboauth-0.9.5 #
  * 1. there is a bug in liboauth-0.9.4:
```
version 0.9.5
 	- fixed issue with decoding already encoded characters
 	in the base-URL (not parameters).
 	reported by L. Alberto Giménez
```
    * discussion for issue of url escape with liboauth author:
```
issue of url escape:
if I give a url to oauth_sign_url2(), the url is escaped by myself.

[2012-03-30 23:30:07] url escaped:
http%3A%2F%2Fopenapi.kuaipan.cn%2F1%2Fmetadata%2Fkuaipan%2F%2Fdemo%2F%E4%B8%AD%E6%96%87%E7%9B%AE%E5%BD%95_02
...

but when call oauth_sign_url2(url, ....), then it will return:
[2012-03-30 23:30:07] return url:
http://openapi.kuaipan.cn/1/metadata/kuaipan//demo/中文目录_02?oauth_consumer_key=xc8D2NfL9c53vkrP&oauth_nonce=Jgj5PGrU_xyioRZeKU5FNBO&oauth_signature_method=HMAC-SHA1&oauth_timestamp=1333121407&oauth_token=32a7d5efb0d5442e8e32c6d7518e8239&oauth_version=1.0&oauth_signature=%2Fk0kgBPAvY%2F1NgNCvOxAgAkWt90%3D<http://openapi.kuaipan.cn/1/metadata/kuaipan//demo/%E4%B8%AD%E6%96%87%E7%9B%AE%E5%BD%95_02?oauth_consumer_key=xc8D2NfL9c53vkrP&oauth_nonce=Jgj5PGrU_xyioRZeKU5FNBO&oauth_signature_method=HMAC-SHA1&oauth_timestamp=1333121407&oauth_token=32a7d5efb0d5442e8e32c6d7518e8239&oauth_version=1.0&oauth_signature=%2Fk0kgBPAvY%2F1NgNCvOxAgAkWt90%3D>
```
    * changeset for kpfs:
      * it could not get chinese dir metadata from kuaipan.
        * http://code.google.com/p/kpfs/source/detail?r=7
  * 2. there is another bug in libcurl, we need to use "--with-curltimeout" to configure liboauth
    * ./configure --enable-nss --with-curltimeout
```
error message:
/tmp/kpfs/demo$ ls
ls: cannot open directory .: Transport endpoint is not connected
crash log:
/work/open_src/kpfs.svn/src/.libs/lt-kpfs terminated
======= Backtrace: =========
/lib/i386-linux-gnu/libc.so.6(__fortify_fail+0x45)[0xb74c7dd5]
/lib/i386-linux-gnu/libc.so.6(+0xffd2a)[0xb74c7d2a]
/lib/i386-linux-gnu/libc.so.6(__longjmp_chk+0x4b)[0xb74c7c9b]
/usr/lib/i386-linux-gnu/libcurl-nss.so.4(+0x9f15)[0xb7577f15]
[0xb7741400]
[0xb7741424]
```
    * how to solve it
      * http://stackoverflow.com/questions/9191668/error-longjmp-causes-uninitialized-stack-frame
      * disable signal in curl by:
        * curl\_easy\_setopt(curl, CURLOPT\_NOSIGNAL, 1);


# How to compile and install liboauth-0.9.5 #
  * Download liboauth-0.9.5
    * http://liboauth.sourceforge.net/
  * compile
    * ./configure --enable-nss --with-curltimeout
    * make
  * install
    * sudo make install
```
$ ls /usr/local/lib/liboauth.* -l
-rw-r--r-- 1 root root 126358 Mar 31 00:52 /usr/local/lib/liboauth.a
-rwxr-xr-x 1 root root   1049 Mar 31 00:52 /usr/local/lib/liboauth.la
lrwxrwxrwx 1 root root     17 Mar 31 00:52 /usr/local/lib/liboauth.so -> liboauth.so.0.8.2
lrwxrwxrwx 1 root root     17 Mar 31 00:52 /usr/local/lib/liboauth.so.0 -> liboauth.so.0.8.2
-rwxr-xr-x 1 root root 110682 Mar 31 00:52 /usr/local/lib/liboauth.so.0.8.2
```
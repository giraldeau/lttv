#!/bin/sh

export CLASSPATH=.:/usr/lib/jvm/java-1.5.0-sun-1.5.0.06/bin

#Sample
javac Sample.java
javah -jni Sample
gcc -I /usr/lib/jvm/java-1.5.0-sun-1.5.0.06/include \
  -I /usr/lib/jvm/java-1.5.0-sun-1.5.0.06/include/linux \
  -shared -Wl,-soname,libltt-java-string \
  -o libltt-java-string.so ltt-java-string.c \
  ../ltt-facility-loader-user_generic.c
LD_LIBRARY_PATH=. java Sample

#TestBrand
echo javac Threadbrand
javac -d . ThreadBrand.java
echo javah Threadbrand
javah -jni ltt.ThreadBrand
echo gcc
gcc -I /usr/lib/jvm/java-1.5.0-sun-1.5.0.06/include \
  -I /usr/lib/jvm/java-1.5.0-sun-1.5.0.06/include/linux \
  -shared -Wl,-soname,libltt-java-thread_brand \
  -o libltt-java-thread_brand.so ltt-java-thread_brand.c \
  ../ltt-facility-loader-user_generic.c
echo javac test
javac TestBrand.java
echo run
LD_LIBRARY_PATH=. java TestBrand

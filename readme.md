# Multi-threaded Socket File Downloader

## Intro

This is an example C++ project that shows how to download files via
HTTP using sockets. It does so using multi-threading, which theoretically
could make the downloads faster due to concurrency.

However, there is an added twist in that the file downloader class instances
have purposely been prohibited from writing directly to disk. I deliberately do
file I/O in the main thread, and the file downloaders block until the main thread
can consume their messages and write out to disk. This is completely unnecessary
and slows down the file downloads, but I wanted to test how to use locks and mutexes
in C++, so that's why I did it this way.

## Building

This project requires C++17 extensions, so please make sure those are installed
before attempting to build this project. In addition, it may not work on Windows
due to windows needing [winsock](https://docs.microsoft.com/en-us/windows/desktop/winsock/creating-a-basic-winsock-application), but should work on *nix systems and Mac OSX.

After everything is installed you should be able to run the default make target
by just running `make` to build. The executable is `concurrency.out`

## Running

The program should be run as follows:

```sh
./concurrency.out <url_1> <filepath_1> <url_2> <filepath_2> ... <url_n> <filepath_n>
```

Example command is below.

```sh
make # builds target
./concurrency.out \
  "http://example.com/img1.jpg" "./img1.jpg" \
  "http://example.com/img2.jpg" "./img2.jpg" \
  "http://example.com/img3.jpg" "./img3.jpg"
```

This command will download three images and copy them to the local directory with the same file names as in the URLs provided.


**Note that you can only download over HTTP.** HTTPS or any URL that redirects
to HTTPS will *not* work.

## Feedback

Feel free to leave any questions in the issues section. Note that this code is
not meant to be fully tested/production ready. It is just a prototype for
demonstrating sockets and multi-threading in C++ with some new C++17 features.

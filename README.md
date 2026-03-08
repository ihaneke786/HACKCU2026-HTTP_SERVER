Data Flow in program

browser request
      ↓
server.c (accept connection)
      ↓
http.c (parse request)
      ↓
cache.c (check cache)
      ↓
filesystem (if cache miss)
      ↓
build response
      ↓
send response


to run----

compile server
gcc -Wall -Wextra -Iinclude src/*.c -o server

run server
./server 8080

test server
http://localhost:8080/
or 
http://localhost:8080/hello.txt

better
curl http://localhost:8080/
curl -v http://localhost:8080/
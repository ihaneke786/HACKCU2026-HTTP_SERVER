Build and Run the Server

1. Compile the server
gcc -Wall -Wextra -Iinclude src/*.c -o server

2. Start the server
./server 8080

3. Tests -----------------------------------------------------
1. Test GET requests (browser)
http://localhost:8080/
or
http://localhost:8080/hello.txt

-----------------------------------------------------
2. Test GET requests (curl)
curl http://localhost:8080/
or verbose mode
curl -v http://localhost:8080/

-----------------------------------------------------
3. Test POST
curl -X POST http://localhost:8080/test -d "hello=world"

-----------------------------------------------------
4. Test PUT
curl -X PUT http://localhost:8080/newfile.txt -d "Hello from PUT"

5. Test DELETE
curl -X DELETE http://localhost:8080/newfile.txt



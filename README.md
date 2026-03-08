Build and Run the Server

1. Compile the server
gcc -Wall -Wextra -pthread -Iinclude src/*.c -o server

2. Start the server
./server 8080

3. Tests 
-----------------------------------------------------

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
3. Test Binary File Serving (Images)
test in browser
http://localhost:8080/images/dog.png
http://localhost:8080/images/strawberry.jpeg

test with curl
curl http://localhost:8080/images/dog.png --output test.png


-----------------------------------------------------
3. Test POST
curl -X POST http://localhost:8080/test -d "hello=world"

-----------------------------------------------------
4. Test PUT
curl -X PUT http://localhost:8080/newfile.txt -d "Hello from PUT"

-----------------------------------------------------
5. Test DELETE
curl -X DELETE http://localhost:8080/newfile.txt





-----------------------------------------------------
Concurrency tests

-----------------------------------------------------
1. basic
curl http://localhost:8080/ &
curl http://localhost:8080/ &
curl http://localhost:8080/ &
curl http://localhost:8080/ &
wait

2. 16 requests at once
for i in {1..16}; do
    curl http://localhost:8080/ &
done
wait


3. Test concurrent image downloads
for i in {1..16}; do
    curl http://localhost:8080/images/dog.png --output dog$i.png &
done
wait

4. Test concurrent mixed requests
curl -X POST http://localhost:8080/test -d "a=1" &
curl -X PUT http://localhost:8080/file1.txt -d "hello" &
curl http://localhost:8080/ &
curl http://localhost:8080/images/dog.png &
curl -X DELETE http://localhost:8080/file1.txt &
wait


5. Stress Test with ApacheBench
brew install httpd
ab -n 200 -c 16 http://localhost:8080/

6. super stress test
ab -n 1000 -c 16 http://localhost:8080/images/dog.png
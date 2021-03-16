openssl genrsa -aes256 -passout pass:iamca -out ca.pass.key 4096
openssl rsa -passin pass:iamca -in ca.pass.key -out ca.key
openssl req -new -x509 -days 365 -new -subj "/CN=localca" -key ca.key -out ca.crt
openssl genrsa -aes256 -passout pass:iamsrv -out server.pass.key 4096
openssl rsa -passin pass:iamsrv -in server.pass.key -out server.key
openssl req -new -subj "/CN=localhost" -key server.key -out server.csr
openssl x509 -CAcreateserial -req -days 365 -in server.csr -CA ca.crt -CAkey ca.key -out server.crt
openssl genrsa -aes256 -passout pass:iamcli -out client.pass.key 4096
openssl rsa -passin pass:iamcli -in client.pass.key -out client.key
openssl req -new -subj "/CN=localhost" -key client.key -out client.csr
openssl x509 -CAcreateserial -req -days 365 -in client.csr -CA ca.crt -CAkey ca.key -out client.crt
del ca.pass.key server.pass.key server.csr client.pass.key client.csr

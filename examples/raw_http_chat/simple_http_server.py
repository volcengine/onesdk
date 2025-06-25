from http.server import BaseHTTPRequestHandler, HTTPServer
import json

class SimpleHTTPRequestHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        # 打印请求头
        print("Request Headers:")
        for header, value in self.headers.items():
            print(f"{header}: {value}")

        # 发送响应
        self.send_response(200)
        self.send_header('Content-type', 'text/plain')
        self.end_headers()
        self.wfile.write(b'GET request received')

    def do_POST(self):
        # 打印请求头
        print("Request Headers:")
        for header, value in self.headers.items():
            print(f"{header}: {value}")

        # 获取请求体
        content_length = int(self.headers['content-length'])
        post_data = self.rfile.read(content_length)
        print("Request Body(len=%d):", len(post_data.decode('utf-8')))
        print(post_data.decode('utf-8'))

        # 发送响应
        self.send_response(200)
        self.send_header('Content-type', 'text/plain')
        self.end_headers()
        self.wfile.write(b'POST request received')

def run(server_class=HTTPServer, handler_class=SimpleHTTPRequestHandler, port=8080):
    server_address = ('', port)
    httpd = server_class(server_address, handler_class)
    print(f'Starting httpd server on port {port}...')
    httpd.serve_forever()

if __name__ == '__main__':
    run()
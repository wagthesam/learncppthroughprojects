#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <iostream>

using tcp = boost::asio::ip::tcp;

void Log(const std::string& ref, const boost::system::error_code& ec)
{
	std::cout << ref << " > "
			  << (ec ? "Error: " : "OK!")
			  << (ec ? ec.message() : "")
			  << std::endl;
}

void OnWrite(
	boost::beast::websocket::stream<boost::beast::tcp_stream>& ws,
	boost::beast::flat_buffer& rBuffer,
	const boost::system::error_code& ec
) {
	if (ec) {
		Log("OnWrite", ec);
		return;
	}
	ws.read(rBuffer);
	std::cout << boost::beast::make_printable(rBuffer.data()) << std::endl;
	ws.close(boost::beast::websocket::close_code::normal);
}

void OnHandshake(
	boost::beast::websocket::stream<boost::beast::tcp_stream>& ws,
	const boost::asio::const_buffer& wBuffer,
	boost::beast::flat_buffer& rBuffer,
	const boost::system::error_code& ec
) {
	if (ec) {
		Log("OnHandshake", ec);
		return;
	}
	ws.text(true);
	ws.async_write(wBuffer, [&ws, &rBuffer](auto ec, auto bytes_transferred){
		OnWrite(ws, rBuffer, ec);
	});
}

void OnConnect(
	boost::beast::websocket::stream<boost::beast::tcp_stream>& ws,
	const std::string& url,
	const std::string& endpoint,
	const boost::asio::const_buffer& wBuffer,
	boost::beast::flat_buffer& rBuffer,
	const boost::system::error_code& ec 
) {
	if (ec) {
		Log("OnConnect", ec);
		return;
	}
	ws.async_handshake(
		url,
		endpoint,
		[&ws, &wBuffer, &rBuffer](auto ec){
			OnHandshake(ws, wBuffer, rBuffer, ec);
		});
}

int main()
{
	const std::string url = "ltnm.learncppthroughprojects.com";
	const std::string port = "80";
	const std::string endpoint = "/echo";
	std::cerr << "start:" << std::this_thread::get_id() << std::endl;
	boost::asio::io_context ioc {};
	tcp::socket socket{boost::asio::make_strand(ioc)};

	boost::system::error_code ec {};
	tcp::resolver resolver {ioc};
	auto resolverIt {resolver.resolve(url, port, ec)};
	if (ec) {
		Log("main", ec);
		return -1;
	}
	boost::beast::websocket::stream<boost::beast::tcp_stream> ws{std::move(socket)};
	const std::string msg = "Hello";
	auto wBuffer = boost::asio::const_buffer(msg.data(), msg.size());
	boost::beast::flat_buffer rBuffer;
	ws.next_layer().async_connect(
		*resolverIt,
		[&ws, &url, &endpoint, &wBuffer, &rBuffer](auto ec) {
			OnConnect(ws, url, endpoint, wBuffer, rBuffer, ec);
		}
	);
	ioc.run();
	return 0;
}

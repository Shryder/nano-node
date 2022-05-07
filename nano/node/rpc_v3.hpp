#pragma once

#include <nano/lib/numbers.hpp>
#include <nano/node/wallet.hpp>
#include <nano/rpc/rpc.hpp>

#include <boost/json.hpp>
#include <boost/json/value.hpp>

#include <boost/property_tree/ptree.hpp>

#include <functional>
#include <string>

namespace nano
{
namespace ipc
{
	class ipc_server;
}
class node;
class node_rpc_config;

class rpc_v3 : public std::enable_shared_from_this<nano::rpc_v3>
{
public:
	using json = boost::json::value;
	enum class API_EC
	{
		INVALID_INPUT = 0,
		MISSING_REQUIRED_FIELD = 1,
		UNSUPPORTED_ACTION = 2
	};

	struct WithError
	{
		bool success;
		std::any value;
	};

	struct ApiError
	{
		int code;
		std::string message = "";
	};

	rpc_v3 (nano::node & node_a, nano::node_rpc_config const & node_rpc_config_a, std::string const & body_a, std::function<void (std::string const &)> const & response_a, std::function<void ()> stop_callback_a);
	void process_request (bool unsafe_a = false);

	boost::json::value prepare_error (ApiError);

	void respond_success (boost::json::value & response_body);
	void respond_error (boost::json::value & error_data);

	boost::json::value handle_action ();

	nano::account nano::rpc_v3::get_account_parameter (std::string const & account);

	boost::json::value account_balance ();

	std::string body;
	json body_json;

	nano::node & node;
	boost::property_tree::ptree request;
	std::function<void (std::string const &)> response;

	std::function<void ()> stop_callback;
	nano::node_rpc_config const & node_rpc_config;
};
}

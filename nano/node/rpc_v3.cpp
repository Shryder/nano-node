#include <nano/lib/config.hpp>
#include <nano/lib/json_error_response.hpp>
#include <nano/lib/timer.hpp>
#include <nano/node/bootstrap/bootstrap_lazy.hpp>
#include <nano/node/common.hpp>
#include <nano/node/election.hpp>
#include <nano/node/json_handler.hpp>
#include <nano/node/node.hpp>
#include <nano/node/node_rpc_config.hpp>
#include <nano/node/rpc_v3.hpp>
#include <nano/node/telemetry.hpp>

#include <boost/json.hpp>

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include <algorithm>
#include <chrono>

using json = nano::rpc_v3::json;
using WithError = nano::rpc_v3::WithError;
using ApiError = nano::rpc_v3::ApiError;

nano::rpc_v3::rpc_v3 (nano::node & node_a, nano::node_rpc_config const & node_rpc_config_a, std::string const & body_a, std::function<void (std::string const &)> const & response_a, std::function<void ()> stop_callback_a) :
	body (body_a),
	node (node_a),
	response (response_a),
	stop_callback (stop_callback_a),
	node_rpc_config (node_rpc_config_a)
{
}

WithError make_error (ApiError & error_data)
{
	// check if error_data.message is set, if not then check if some default generic message exists in some map
	return WithError{ false, error_data };
}

WithError make_success (std::any const & value)
{
	return WithError{ true, value };
}

void nano::rpc_v3::respond_success (boost::json::value & response_body)
{
	boost::json::object response_json = boost::json::object({
		{ "success", true },
		{ "data", response_body }
	});

	this->response (boost::json::serialize (response_json));
}

boost::json::value nano::rpc_v3::prepare_error (ApiError api_error)
{
	return {
		{ "success", false },
		{ "error", { { "code", api_error.code }, { "message", api_error.message } } }
	};
}

void nano::rpc_v3::respond_error (boost::json::value & error_data)
{
	this->response (boost::json::serialize (error_data));
}

nano::account nano::rpc_v3::get_account_parameter (std::string const & field = "account")
{
	auto account_field = body_json.at (field);
	if (!account_field.is_string ())
	{
		throw ApiError{ int(API_EC::INVALID_INPUT), "Account field has to be a string" };
	}

	std::string account_text = boost::json::value_to<std::string> (account_field);
	if (account_text.empty ())
	{
		throw ApiError{ int(API_EC::MISSING_REQUIRED_FIELD), "Account field is required" };
	}

	std::cout << "Account:" << account_text << std::endl;
	nano::account result (0);
	if (!result.decode_account (account_text.c_str ()))
	{
		return result;
	}

	throw ApiError{ int(API_EC::INVALID_INPUT), "Invalid account provided" };
}

boost::json::value nano::rpc_v3::account_balance ()
{
	auto account = get_account_parameter ();
	auto balance (node.balance_pending (account, true));

	return {
		{ "balance", balance.first.convert_to<std::string> () },
		{ "pending", balance.second.convert_to<std::string> () }
	};
}

boost::json::value nano::rpc_v3::handle_action ()
{
	auto action = body_json.at ("action").as_string ();

	if (action == "account_balance")
	{
		return account_balance ();
	}

	throw ApiError{ int(API_EC::UNSUPPORTED_ACTION), "Action is not supported" };
}

void nano::rpc_v3::process_request (bool unsafe_a)
{
	boost::system::error_code ec;
	body_json = boost::json::parse (body, ec);
	if (ec)
	{
		return this->respond_error (this->prepare_error ({ int(API_EC::INVALID_INPUT), "Failed parsing JSON" }));
	}

	try
	{
		auto response = this->handle_action ();

		return this->respond_success (response);
	}
	catch (ApiError err)
	{
		return this->respond_error (this->prepare_error (err));
	}
}

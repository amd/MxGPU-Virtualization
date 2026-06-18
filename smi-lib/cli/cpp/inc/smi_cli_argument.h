/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <string>
#include <map>
#include <vector>

class SmiCliArgument
{
public:
	SmiCliArgument(const std::string& short_name,
				   const std::string& long_name,
				   const std::string& description,
				   const std::string& value_placeholder = "",
				   bool specific = false);

	std::string get_usage() const;
	std::string get_arguments() const;
	static std::string get_description_continuation_indent();
	static std::string get_usage_line_break();

	const std::string& get_short_name() const
	{
		return short_name_;
	}
	const std::string& get_long_name() const
	{
		return long_name_;
	}
	const std::string& get_description() const
	{
		return description_;
	}
	const std::string& get_value_placeholder() const
	{
		return value_placeholder_;
	}
	bool is_specific() const
	{
		return specific_;
	}
	void update_description(const std::string& value)
	{
		description_ = value;
	}
	static constexpr int DESCRIPTION_COLUMN_WIDTH = 60;
	static constexpr int REGULAR_ARG_INDENT = 4;
	static constexpr int SPECIFIC_ARG_INDENT = 8;
	static constexpr int USAGE_LINE_BREAK_INDENT = 21;

private:
	std::string short_name_;
	std::string long_name_;
	std::string description_;
	std::string value_placeholder_;
	bool specific_;
};

class SmiCliHelpCommand
{
public:
	SmiCliHelpCommand(const std::string& command_name, const std::string& description);

	std::string get_help_line() const;

	const std::string& get_command_name() const
	{
		return command_name_;
	}
	const std::string& get_description() const
	{
		return description_;
	}

private:
	std::string command_name_;
	std::string description_;

	static constexpr int COMMAND_COLUMN_WIDTH = 22;
};

class SmiCliArgumentFactory
{
public:
	static SmiCliArgumentFactory& getInstance();

	void initialize(std::vector<std::string> options);

	const SmiCliArgument* get_argument(const std::string& key) const;

	std::string get_usage_string(const std::vector<std::string>& keys) const;
	std::string get_usage_string_with_formatting(const std::vector<std::string>& keys,
			int starting_row_count) const;
	std::string get_arguments_string(const std::vector<std::string>& keys) const;

	const SmiCliHelpCommand* get_help_command(const std::string& command_name) const;
	std::string get_help_commands_string(const std::vector<std::string>& command_names) const;

	std::string get_format_usage(const std::vector<std::string>& format_keys) const;
	std::string get_format_arguments(const std::vector<std::string>& format_keys) const;
	bool update_argument_description(const std::string& key, const std::string& new_description);

private:
	SmiCliArgumentFactory() = default;
	std::map<std::string, SmiCliArgument> arguments_;
	std::map<std::string, SmiCliHelpCommand> help_commands_;

	void initialize_static_arguments();
	void initialize_metric_arguments();
	void initialize_topology_arguments();
	void initialize_device_arguments();
	void initialize_help_commands();
};

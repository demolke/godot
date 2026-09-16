/**************************************************************************/
/*  test_tools.h                                                          */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#pragma once

#include "core/core_globals.h"
#include "core/error/error_macros.h"
#include "core/string/ustring.h"
#include "core/templates/vector.h"
#include "tests/test_macros.h"

#include <exception>

struct ErrorDetector {
	ErrorDetector() {
		eh.errfunc = _detect_error;
		eh.userdata = this;

		add_error_handler(&eh);
	}

	~ErrorDetector() {
		remove_error_handler(&eh);
	}

	void clear() {
		has_error = false;
		messages.clear();
	}

	bool has_message_containing(const String &p_text) const {
		for (const String &msg : messages) {
			if (msg.contains(p_text)) {
				return true;
			}
		}
		return false;
	}

	static void _detect_error(void *p_self, const char *p_func, const char *p_file, int p_line, const char *p_error, const char *p_errorexp, bool p_editor_notify, ErrorHandlerType p_type) {
		ErrorDetector *self = (ErrorDetector *)p_self;
		self->has_error = true;
		String message;
		if (p_error != nullptr && p_error[0] != '\0') {
			message += p_error;
		}
		if (p_errorexp != nullptr && p_errorexp[0] != '\0') {
			if (!message.is_empty()) {
				message += "\n";
			}
			message += p_errorexp;
		}
		self->messages.push_back(message);
	}

	ErrorHandlerList eh;
	bool has_error = false;
	Vector<String> messages;
};

// Scoped expectation for engine errors.
struct ExpectErrors : ErrorDetector {
	ExpectErrors(const Vector<String> &p_expected_messages) :
			expected(p_expected_messages) {
		was_print_enabled = CoreGlobals::print_error_enabled;
		CoreGlobals::print_error_enabled = false;
	}

	ExpectErrors(const String &p_expected_message) {
		expected.push_back(p_expected_message);
		was_print_enabled = CoreGlobals::print_error_enabled;
		CoreGlobals::print_error_enabled = false;
	}

	~ExpectErrors() {
		CoreGlobals::print_error_enabled = was_print_enabled;
		if (std::uncaught_exceptions() == 0) {
			CHECK(has_error);
			for (const String &text : expected) {
				CHECK_MESSAGE(has_message_containing(text), "An expected engine error was not emitted.");
			}
		}
	}

	ExpectErrors(const ExpectErrors &) = delete;
	ExpectErrors &operator=(const ExpectErrors &) = delete;

	Vector<String> expected;
	bool was_print_enabled = true;
};

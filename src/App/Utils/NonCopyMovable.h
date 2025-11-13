#pragma once

// clang-format off
// NOLINTBEGIN(bugprone-macro-parentheses)

#define NON_COPYABLE(NAME)                   \
											 \
	NAME(const NAME &) = delete;             \
	NAME & operator=(const NAME &) = delete; \

#define NON_MOVABLE(NAME)                        \
												 \
	NAME(NAME &&) noexcept = delete;             \
	NAME & operator=(NAME &&) noexcept = delete; \

// rule 5
#define NON_COPY_MOVABLE(NAME) NON_COPYABLE(NAME) NON_MOVABLE(NAME)

// NOLINTEND(bugprone-macro-parentheses)
// clang-format on

#pragma once

constexpr static auto DEF_FORM_SYM              = "def!";
constexpr static auto DEFMACRO_FORM_SYM         = "defmacro!";
constexpr static auto LET_STAR_FORM_SYM         = "let*";
constexpr static auto DO_FORM_SYM               = "do";
constexpr static auto IF_FORM_SYM               = "if";
constexpr static auto FN_STAR_FORM_SYM          = "fn*";
constexpr static auto QUOTE_FORM_SYM            = "quote";
constexpr static auto QUASIQUOTEEXPAND_FORM_SYM = "quasiquoteexpand";
constexpr static auto QUASIQUOTE_FORM_SYM       = "quasiquote";
constexpr static auto UNQUOTE_FORM_SYM          = "unquote";
constexpr static auto SPLICE_UNQUOTE_FORM_SYM   = "splice-unquote";
constexpr static auto MACROEXPAND_FORM_SYM      = "macroexpand";
constexpr static auto TRY_STAR_FORM_SYM         = "try*";
constexpr static auto CATCH_STAR_FORM_SYM       = "catch*";

constexpr static auto DEREF_FN_SYM              = "deref";
constexpr static auto LIST_FN_SYM               = "list";
constexpr static auto VEC_FN_SYM                = "vec";
constexpr static auto CONS_FN_SYM               = "cons";
constexpr static auto CONCAT_FN_SYM             = "concat";
constexpr static auto SWAP_BANG_FN_SYM          = "swap!";
constexpr static auto WITH_META_FN_SYM          = "with-meta";
constexpr static auto APPLY_FN_SYM              = "apply";

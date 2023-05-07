//
// Created by whalien on 07/05/23.
//

#ifndef MB_INCLUDE_MERGEBOT_PARSER_LANGUAGES_CPP_H
#define MB_INCLUDE_MERGEBOT_PARSER_LANGUAGES_CPP_H
#include "mergebot/parser/field.h"
#include "mergebot/parser/symbol.h"
#include "tree_sitter/api.h"

extern "C" const TSLanguage *tree_sitter_cpp(void);

namespace mergebot {

namespace ts {

namespace cpp {

inline const TSLanguage *language(void) { return tree_sitter_cpp(); }

namespace symbols {
constexpr Symbol ts_builtin_sym_end = Symbol(0, "end");
constexpr Symbol sym_identifier = Symbol(1, "identifier");
constexpr Symbol aux_sym_preproc_include_token1 = Symbol(2, "#include");
constexpr Symbol anon_sym_LF = Symbol(3, "\n");
constexpr Symbol aux_sym_preproc_def_token1 = Symbol(4, "#define");
constexpr Symbol anon_sym_LPAREN = Symbol(5, "(");
constexpr Symbol anon_sym_DOT_DOT_DOT = Symbol(6, "...");
constexpr Symbol anon_sym_COMMA = Symbol(7, ",");
constexpr Symbol anon_sym_RPAREN = Symbol(8, ")");
constexpr Symbol aux_sym_preproc_if_token1 = Symbol(9, "#if");
constexpr Symbol aux_sym_preproc_if_token2 = Symbol(10, "#endif");
constexpr Symbol aux_sym_preproc_ifdef_token1 = Symbol(11, "#ifdef");
constexpr Symbol aux_sym_preproc_ifdef_token2 = Symbol(12, "#ifndef");
constexpr Symbol aux_sym_preproc_else_token1 = Symbol(13, "#else");
constexpr Symbol aux_sym_preproc_elif_token1 = Symbol(14, "#elif");
constexpr Symbol sym_preproc_directive = Symbol(15, "preproc_directive");
constexpr Symbol sym_preproc_arg = Symbol(16, "preproc_arg");
constexpr Symbol anon_sym_LPAREN2 = Symbol(17, "(");
constexpr Symbol anon_sym_defined = Symbol(18, "defined");
constexpr Symbol anon_sym_BANG = Symbol(19, "!");
constexpr Symbol anon_sym_TILDE = Symbol(20, "~");
constexpr Symbol anon_sym_DASH = Symbol(21, "-");
constexpr Symbol anon_sym_PLUS = Symbol(22, "+");
constexpr Symbol anon_sym_STAR = Symbol(23, "*");
constexpr Symbol anon_sym_SLASH = Symbol(24, "/");
constexpr Symbol anon_sym_PERCENT = Symbol(25, "%");
constexpr Symbol anon_sym_PIPE_PIPE = Symbol(26, "||");
constexpr Symbol anon_sym_AMP_AMP = Symbol(27, "&&");
constexpr Symbol anon_sym_PIPE = Symbol(28, "|");
constexpr Symbol anon_sym_CARET = Symbol(29, "^");
constexpr Symbol anon_sym_AMP = Symbol(30, "&");
constexpr Symbol anon_sym_EQ_EQ = Symbol(31, "==");
constexpr Symbol anon_sym_BANG_EQ = Symbol(32, "!=");
constexpr Symbol anon_sym_GT = Symbol(33, ">");
constexpr Symbol anon_sym_GT_EQ = Symbol(34, ">=");
constexpr Symbol anon_sym_LT_EQ = Symbol(35, "<=");
constexpr Symbol anon_sym_LT = Symbol(36, "<");
constexpr Symbol anon_sym_LT_LT = Symbol(37, "<<");
constexpr Symbol anon_sym_GT_GT = Symbol(38, ">>");
constexpr Symbol anon_sym_SEMI = Symbol(39, ";");
constexpr Symbol anon_sym_typedef = Symbol(40, "typedef");
constexpr Symbol anon_sym_extern = Symbol(41, "extern");
constexpr Symbol anon_sym___attribute__ = Symbol(42, "__attribute__");
constexpr Symbol anon_sym_COLON_COLON = Symbol(43, "::");
constexpr Symbol anon_sym_LBRACK_LBRACK = Symbol(44, "[[");
constexpr Symbol anon_sym_RBRACK_RBRACK = Symbol(45, "]]");
constexpr Symbol anon_sym___declspec = Symbol(46, "__declspec");
constexpr Symbol anon_sym___based = Symbol(47, "__based");
constexpr Symbol anon_sym___cdecl = Symbol(48, "__cdecl");
constexpr Symbol anon_sym___clrcall = Symbol(49, "__clrcall");
constexpr Symbol anon_sym___stdcall = Symbol(50, "__stdcall");
constexpr Symbol anon_sym___fastcall = Symbol(51, "__fastcall");
constexpr Symbol anon_sym___thiscall = Symbol(52, "__thiscall");
constexpr Symbol anon_sym___vectorcall = Symbol(53, "__vectorcall");
constexpr Symbol sym_ms_restrict_modifier = Symbol(54, "ms_restrict_modifier");
constexpr Symbol sym_ms_unsigned_ptr_modifier =
    Symbol(55, "ms_unsigned_ptr_modifier");
constexpr Symbol sym_ms_signed_ptr_modifier =
    Symbol(56, "ms_signed_ptr_modifier");
constexpr Symbol anon_sym__unaligned = Symbol(57, "_unaligned");
constexpr Symbol anon_sym___unaligned = Symbol(58, "__unaligned");
constexpr Symbol anon_sym_LBRACE = Symbol(59, "{");
constexpr Symbol anon_sym_RBRACE = Symbol(60, "}");
constexpr Symbol anon_sym_LBRACK = Symbol(61, "[");
constexpr Symbol anon_sym_RBRACK = Symbol(62, "]");
constexpr Symbol anon_sym_EQ = Symbol(63, "=");
constexpr Symbol anon_sym_static = Symbol(64, "static");
constexpr Symbol anon_sym_register = Symbol(65, "register");
constexpr Symbol anon_sym_inline = Symbol(66, "inline");
constexpr Symbol anon_sym_thread_local = Symbol(67, "thread_local");
constexpr Symbol anon_sym_const = Symbol(68, "const");
constexpr Symbol anon_sym_volatile = Symbol(69, "volatile");
constexpr Symbol anon_sym_restrict = Symbol(70, "restrict");
constexpr Symbol anon_sym__Atomic = Symbol(71, "_Atomic");
constexpr Symbol anon_sym_mutable = Symbol(72, "mutable");
constexpr Symbol anon_sym_constexpr = Symbol(73, "constexpr");
constexpr Symbol anon_sym_constinit = Symbol(74, "constinit");
constexpr Symbol anon_sym_consteval = Symbol(75, "consteval");
constexpr Symbol anon_sym_signed = Symbol(76, "signed");
constexpr Symbol anon_sym_unsigned = Symbol(77, "unsigned");
constexpr Symbol anon_sym_long = Symbol(78, "long");
constexpr Symbol anon_sym_short = Symbol(79, "short");
constexpr Symbol sym_primitive_type = Symbol(80, "primitive_type");
constexpr Symbol anon_sym_enum = Symbol(81, "enum");
constexpr Symbol anon_sym_class = Symbol(82, "class");
constexpr Symbol anon_sym_struct = Symbol(83, "struct");
constexpr Symbol anon_sym_union = Symbol(84, "union");
constexpr Symbol anon_sym_COLON = Symbol(85, ":");
constexpr Symbol anon_sym_if = Symbol(86, "if");
constexpr Symbol anon_sym_else = Symbol(87, "else");
constexpr Symbol anon_sym_switch = Symbol(88, "switch");
constexpr Symbol anon_sym_case = Symbol(89, "case");
constexpr Symbol anon_sym_default = Symbol(90, "default");
constexpr Symbol anon_sym_while = Symbol(91, "while");
constexpr Symbol anon_sym_do = Symbol(92, "do");
constexpr Symbol anon_sym_for = Symbol(93, "for");
constexpr Symbol anon_sym_return = Symbol(94, "return");
constexpr Symbol anon_sym_break = Symbol(95, "break");
constexpr Symbol anon_sym_continue = Symbol(96, "continue");
constexpr Symbol anon_sym_goto = Symbol(97, "goto");
constexpr Symbol anon_sym_QMARK = Symbol(98, "\?");
constexpr Symbol anon_sym_STAR_EQ = Symbol(99, "*=");
constexpr Symbol anon_sym_SLASH_EQ = Symbol(100, "/=");
constexpr Symbol anon_sym_PERCENT_EQ = Symbol(101, "%=");
constexpr Symbol anon_sym_PLUS_EQ = Symbol(102, "+=");
constexpr Symbol anon_sym_DASH_EQ = Symbol(103, "-=");
constexpr Symbol anon_sym_LT_LT_EQ = Symbol(104, "<<=");
constexpr Symbol anon_sym_GT_GT_EQ = Symbol(105, ">>=");
constexpr Symbol anon_sym_AMP_EQ = Symbol(106, "&=");
constexpr Symbol anon_sym_CARET_EQ = Symbol(107, "^=");
constexpr Symbol anon_sym_PIPE_EQ = Symbol(108, "|=");
constexpr Symbol anon_sym_and_eq = Symbol(109, "and_eq");
constexpr Symbol anon_sym_or_eq = Symbol(110, "or_eq");
constexpr Symbol anon_sym_xor_eq = Symbol(111, "xor_eq");
constexpr Symbol anon_sym_not = Symbol(112, "not");
constexpr Symbol anon_sym_compl = Symbol(113, "compl");
constexpr Symbol anon_sym_LT_EQ_GT = Symbol(114, "<=>");
constexpr Symbol anon_sym_or = Symbol(115, "or");
constexpr Symbol anon_sym_and = Symbol(116, "and");
constexpr Symbol anon_sym_bitor = Symbol(117, "bitor");
constexpr Symbol anon_sym_xor = Symbol(118, "xor");
constexpr Symbol anon_sym_bitand = Symbol(119, "bitand");
constexpr Symbol anon_sym_not_eq = Symbol(120, "not_eq");
constexpr Symbol anon_sym_DASH_DASH = Symbol(121, "--");
constexpr Symbol anon_sym_PLUS_PLUS = Symbol(122, "++");
constexpr Symbol anon_sym_sizeof = Symbol(123, "sizeof");
constexpr Symbol anon_sym_DOT = Symbol(124, ".");
constexpr Symbol anon_sym_DASH_GT = Symbol(125, "->");
constexpr Symbol sym_number_literal = Symbol(126, "number_literal");
constexpr Symbol anon_sym_L_SQUOTE = Symbol(127, "L'");
constexpr Symbol anon_sym_u_SQUOTE = Symbol(128, "u'");
constexpr Symbol anon_sym_U_SQUOTE = Symbol(129, "U'");
constexpr Symbol anon_sym_u8_SQUOTE = Symbol(130, "u8'");
constexpr Symbol anon_sym_SQUOTE = Symbol(131, "'");
constexpr Symbol aux_sym_char_literal_token1 =
    Symbol(132, "char_literal_token1");
constexpr Symbol anon_sym_L_DQUOTE = Symbol(133, "L\"");
constexpr Symbol anon_sym_u_DQUOTE = Symbol(134, "u\"");
constexpr Symbol anon_sym_U_DQUOTE = Symbol(135, "U\"");
constexpr Symbol anon_sym_u8_DQUOTE = Symbol(136, "u8\"");
constexpr Symbol anon_sym_DQUOTE = Symbol(137, "\"");
constexpr Symbol aux_sym_string_literal_token1 =
    Symbol(138, "string_literal_token1");
constexpr Symbol sym_escape_sequence = Symbol(139, "escape_sequence");
constexpr Symbol sym_system_lib_string = Symbol(140, "system_lib_string");
constexpr Symbol sym_true = Symbol(141, "true");
constexpr Symbol sym_false = Symbol(142, "false");
constexpr Symbol sym_null = Symbol(143, "null");
constexpr Symbol sym_comment = Symbol(144, "comment");
constexpr Symbol sym_auto = Symbol(145, "auto");
constexpr Symbol anon_sym_decltype = Symbol(146, "decltype");
constexpr Symbol anon_sym_final = Symbol(147, "final");
constexpr Symbol anon_sym_override = Symbol(148, "override");
constexpr Symbol anon_sym_virtual = Symbol(149, "virtual");
constexpr Symbol anon_sym_explicit = Symbol(150, "explicit");
constexpr Symbol anon_sym_typename = Symbol(151, "typename");
constexpr Symbol anon_sym_template = Symbol(152, "template");
constexpr Symbol anon_sym_GT2 = Symbol(153, ">");
constexpr Symbol anon_sym_operator = Symbol(154, "operator");
constexpr Symbol anon_sym_try = Symbol(155, "try");
constexpr Symbol anon_sym_delete = Symbol(156, "delete");
constexpr Symbol anon_sym_friend = Symbol(157, "friend");
constexpr Symbol anon_sym_public = Symbol(158, "public");
constexpr Symbol anon_sym_private = Symbol(159, "private");
constexpr Symbol anon_sym_protected = Symbol(160, "protected");
constexpr Symbol anon_sym_noexcept = Symbol(161, "noexcept");
constexpr Symbol anon_sym_throw = Symbol(162, "throw");
constexpr Symbol anon_sym_namespace = Symbol(163, "namespace");
constexpr Symbol anon_sym_using = Symbol(164, "using");
constexpr Symbol anon_sym_static_assert = Symbol(165, "static_assert");
constexpr Symbol anon_sym_concept = Symbol(166, "concept");
constexpr Symbol anon_sym_co_return = Symbol(167, "co_return");
constexpr Symbol anon_sym_co_yield = Symbol(168, "co_yield");
constexpr Symbol anon_sym_catch = Symbol(169, "catch");
constexpr Symbol anon_sym_R_DQUOTE = Symbol(170, "R\"");
constexpr Symbol anon_sym_LR_DQUOTE = Symbol(171, "LR\"");
constexpr Symbol anon_sym_uR_DQUOTE = Symbol(172, "uR\"");
constexpr Symbol anon_sym_UR_DQUOTE = Symbol(173, "UR\"");
constexpr Symbol anon_sym_u8R_DQUOTE = Symbol(174, "u8R\"");
constexpr Symbol anon_sym_co_await = Symbol(175, "co_await");
constexpr Symbol anon_sym_new = Symbol(176, "new");
constexpr Symbol anon_sym_requires = Symbol(177, "requires");
constexpr Symbol anon_sym_DOT_STAR = Symbol(178, ".*");
constexpr Symbol anon_sym_DASH_GT_STAR = Symbol(179, "->*");
constexpr Symbol anon_sym_LPAREN_RPAREN = Symbol(180, "()");
constexpr Symbol anon_sym_LBRACK_RBRACK = Symbol(181, "[]");
constexpr Symbol anon_sym_DQUOTE_DQUOTE = Symbol(182, "\"\"");
constexpr Symbol sym_this = Symbol(183, "this");
constexpr Symbol sym_nullptr = Symbol(184, "nullptr");
constexpr Symbol sym_literal_suffix = Symbol(185, "literal_suffix");
constexpr Symbol sym_raw_string_delimiter = Symbol(186, "raw_string_delimiter");
constexpr Symbol sym_raw_string_content = Symbol(187, "raw_string_content");
constexpr Symbol sym_translation_unit = Symbol(188, "translation_unit");
constexpr Symbol sym_preproc_include = Symbol(189, "preproc_include");
constexpr Symbol sym_preproc_def = Symbol(190, "preproc_def");
constexpr Symbol sym_preproc_function_def = Symbol(191, "preproc_function_def");
constexpr Symbol sym_preproc_params = Symbol(192, "preproc_params");
constexpr Symbol sym_preproc_call = Symbol(193, "preproc_call");
constexpr Symbol sym_preproc_if = Symbol(194, "preproc_if");
constexpr Symbol sym_preproc_ifdef = Symbol(195, "preproc_ifdef");
constexpr Symbol sym_preproc_else = Symbol(196, "preproc_else");
constexpr Symbol sym_preproc_elif = Symbol(197, "preproc_elif");
constexpr Symbol sym_preproc_if_in_field_declaration_list =
    Symbol(198, "preproc_if");
constexpr Symbol sym_preproc_ifdef_in_field_declaration_list =
    Symbol(199, "preproc_ifdef");
constexpr Symbol sym_preproc_else_in_field_declaration_list =
    Symbol(200, "preproc_else");
constexpr Symbol sym_preproc_elif_in_field_declaration_list =
    Symbol(201, "preproc_elif");
constexpr Symbol sym__preproc_expression = Symbol(202, "_preproc_expression");
constexpr Symbol sym_preproc_parenthesized_expression =
    Symbol(203, "parenthesized_expression");
constexpr Symbol sym_preproc_defined = Symbol(204, "preproc_defined");
constexpr Symbol sym_preproc_unary_expression = Symbol(205, "unary_expression");
constexpr Symbol sym_preproc_call_expression = Symbol(206, "call_expression");
constexpr Symbol sym_preproc_argument_list = Symbol(207, "argument_list");
constexpr Symbol sym_preproc_binary_expression =
    Symbol(208, "binary_expression");
constexpr Symbol sym_function_definition = Symbol(209, "function_definition");
constexpr Symbol sym_declaration = Symbol(210, "declaration");
constexpr Symbol sym_type_definition = Symbol(211, "type_definition");
constexpr Symbol sym__declaration_modifiers =
    Symbol(212, "_declaration_modifiers");
constexpr Symbol sym__declaration_specifiers =
    Symbol(213, "_declaration_specifiers");
constexpr Symbol sym_linkage_specification =
    Symbol(214, "linkage_specification");
constexpr Symbol sym_attribute_specifier = Symbol(215, "attribute_specifier");
constexpr Symbol sym_attribute = Symbol(216, "attribute");
constexpr Symbol sym_attribute_declaration =
    Symbol(217, "attribute_declaration");
constexpr Symbol sym_ms_declspec_modifier = Symbol(218, "ms_declspec_modifier");
constexpr Symbol sym_ms_based_modifier = Symbol(219, "ms_based_modifier");
constexpr Symbol sym_ms_call_modifier = Symbol(220, "ms_call_modifier");
constexpr Symbol sym_ms_unaligned_ptr_modifier =
    Symbol(221, "ms_unaligned_ptr_modifier");
constexpr Symbol sym_ms_pointer_modifier = Symbol(222, "ms_pointer_modifier");
constexpr Symbol sym_declaration_list = Symbol(223, "declaration_list");
constexpr Symbol sym__declarator = Symbol(224, "_declarator");
constexpr Symbol sym__field_declarator = Symbol(225, "_field_declarator");
constexpr Symbol sym__type_declarator = Symbol(226, "_type_declarator");
constexpr Symbol sym__abstract_declarator = Symbol(227, "_abstract_declarator");
constexpr Symbol sym_parenthesized_declarator =
    Symbol(228, "parenthesized_declarator");
constexpr Symbol sym_parenthesized_field_declarator =
    Symbol(229, "parenthesized_declarator");
constexpr Symbol sym_parenthesized_type_declarator =
    Symbol(230, "parenthesized_declarator");
constexpr Symbol sym_abstract_parenthesized_declarator =
    Symbol(231, "abstract_parenthesized_declarator");
constexpr Symbol sym_attributed_declarator =
    Symbol(232, "attributed_declarator");
constexpr Symbol sym_attributed_field_declarator =
    Symbol(233, "attributed_declarator");
constexpr Symbol sym_attributed_type_declarator =
    Symbol(234, "attributed_declarator");
constexpr Symbol sym_pointer_declarator = Symbol(235, "pointer_declarator");
constexpr Symbol sym_pointer_field_declarator =
    Symbol(236, "pointer_declarator");
constexpr Symbol sym_pointer_type_declarator =
    Symbol(237, "pointer_declarator");
constexpr Symbol sym_abstract_pointer_declarator =
    Symbol(238, "abstract_pointer_declarator");
constexpr Symbol sym_function_declarator = Symbol(239, "function_declarator");
constexpr Symbol sym_function_field_declarator =
    Symbol(240, "function_declarator");
constexpr Symbol sym_function_type_declarator =
    Symbol(241, "function_declarator");
constexpr Symbol sym_abstract_function_declarator =
    Symbol(242, "abstract_function_declarator");
constexpr Symbol sym_array_declarator = Symbol(243, "array_declarator");
constexpr Symbol sym_array_field_declarator = Symbol(244, "array_declarator");
constexpr Symbol sym_array_type_declarator = Symbol(245, "array_declarator");
constexpr Symbol sym_abstract_array_declarator =
    Symbol(246, "abstract_array_declarator");
constexpr Symbol sym_init_declarator = Symbol(247, "init_declarator");
constexpr Symbol sym_compound_statement = Symbol(248, "compound_statement");
constexpr Symbol sym_storage_class_specifier =
    Symbol(249, "storage_class_specifier");
constexpr Symbol sym_type_qualifier = Symbol(250, "type_qualifier");
constexpr Symbol sym__type_specifier = Symbol(251, "_type_specifier");
constexpr Symbol sym_sized_type_specifier = Symbol(252, "sized_type_specifier");
constexpr Symbol sym_enum_specifier = Symbol(253, "enum_specifier");
constexpr Symbol sym_enumerator_list = Symbol(254, "enumerator_list");
constexpr Symbol sym_struct_specifier = Symbol(255, "struct_specifier");
constexpr Symbol sym_union_specifier = Symbol(256, "union_specifier");
constexpr Symbol sym_field_declaration_list =
    Symbol(257, "field_declaration_list");
constexpr Symbol sym__field_declaration_list_item =
    Symbol(258, "_field_declaration_list_item");
constexpr Symbol sym_field_declaration = Symbol(259, "field_declaration");
constexpr Symbol sym_bitfield_clause = Symbol(260, "bitfield_clause");
constexpr Symbol sym_enumerator = Symbol(261, "enumerator");
constexpr Symbol sym_parameter_list = Symbol(262, "parameter_list");
constexpr Symbol sym_parameter_declaration =
    Symbol(263, "parameter_declaration");
constexpr Symbol sym_attributed_statement = Symbol(264, "attributed_statement");
constexpr Symbol sym_labeled_statement = Symbol(265, "labeled_statement");
constexpr Symbol sym_expression_statement = Symbol(266, "expression_statement");
constexpr Symbol sym_if_statement = Symbol(267, "if_statement");
constexpr Symbol sym_switch_statement = Symbol(268, "switch_statement");
constexpr Symbol sym_case_statement = Symbol(269, "case_statement");
constexpr Symbol sym_while_statement = Symbol(270, "while_statement");
constexpr Symbol sym_do_statement = Symbol(271, "do_statement");
constexpr Symbol sym_for_statement = Symbol(272, "for_statement");
constexpr Symbol sym_return_statement = Symbol(273, "return_statement");
constexpr Symbol sym_break_statement = Symbol(274, "break_statement");
constexpr Symbol sym_continue_statement = Symbol(275, "continue_statement");
constexpr Symbol sym_goto_statement = Symbol(276, "goto_statement");
constexpr Symbol sym__expression = Symbol(277, "_expression");
constexpr Symbol sym_comma_expression = Symbol(278, "comma_expression");
constexpr Symbol sym_conditional_expression =
    Symbol(279, "conditional_expression");
constexpr Symbol sym_assignment_expression =
    Symbol(280, "assignment_expression");
constexpr Symbol sym_pointer_expression = Symbol(281, "pointer_expression");
constexpr Symbol sym_unary_expression = Symbol(282, "unary_expression");
constexpr Symbol sym_binary_expression = Symbol(283, "binary_expression");
constexpr Symbol sym_update_expression = Symbol(284, "update_expression");
constexpr Symbol sym_cast_expression = Symbol(285, "cast_expression");
constexpr Symbol sym_type_descriptor = Symbol(286, "type_descriptor");
constexpr Symbol sym_sizeof_expression = Symbol(287, "sizeof_expression");
constexpr Symbol sym_subscript_expression = Symbol(288, "subscript_expression");
constexpr Symbol sym_call_expression = Symbol(289, "call_expression");
constexpr Symbol sym_argument_list = Symbol(290, "argument_list");
constexpr Symbol sym_field_expression = Symbol(291, "field_expression");
constexpr Symbol sym_compound_literal_expression =
    Symbol(292, "compound_literal_expression");
constexpr Symbol sym_parenthesized_expression =
    Symbol(293, "parenthesized_expression");
constexpr Symbol sym_initializer_list = Symbol(294, "initializer_list");
constexpr Symbol sym_initializer_pair = Symbol(295, "initializer_pair");
constexpr Symbol sym_subscript_designator = Symbol(296, "subscript_designator");
constexpr Symbol sym_field_designator = Symbol(297, "field_designator");
constexpr Symbol sym_char_literal = Symbol(298, "char_literal");
constexpr Symbol sym_concatenated_string = Symbol(299, "concatenated_string");
constexpr Symbol sym_string_literal = Symbol(300, "string_literal");
constexpr Symbol sym__empty_declaration = Symbol(301, "_empty_declaration");
constexpr Symbol sym_placeholder_type_specifier =
    Symbol(302, "placeholder_type_specifier");
constexpr Symbol sym_decltype_auto = Symbol(303, "decltype");
constexpr Symbol sym_decltype = Symbol(304, "decltype");
constexpr Symbol sym__class_declaration = Symbol(305, "_class_declaration");
constexpr Symbol sym_class_specifier = Symbol(306, "class_specifier");
constexpr Symbol sym__class_name = Symbol(307, "_class_name");
constexpr Symbol sym_virtual_specifier = Symbol(308, "virtual_specifier");
constexpr Symbol sym_virtual = Symbol(309, "virtual");
constexpr Symbol sym_explicit_function_specifier =
    Symbol(310, "explicit_function_specifier");
constexpr Symbol sym_base_class_clause = Symbol(311, "base_class_clause");
constexpr Symbol sym__enum_base_clause = Symbol(312, "_enum_base_clause");
constexpr Symbol sym_dependent_type = Symbol(313, "dependent_type");
constexpr Symbol sym_template_declaration = Symbol(314, "template_declaration");
constexpr Symbol sym_template_instantiation =
    Symbol(315, "template_instantiation");
constexpr Symbol sym_template_parameter_list =
    Symbol(316, "template_parameter_list");
constexpr Symbol sym_type_parameter_declaration =
    Symbol(317, "type_parameter_declaration");
constexpr Symbol sym_variadic_type_parameter_declaration =
    Symbol(318, "variadic_type_parameter_declaration");
constexpr Symbol sym_optional_type_parameter_declaration =
    Symbol(319, "optional_type_parameter_declaration");
constexpr Symbol sym_template_template_parameter_declaration =
    Symbol(320, "template_template_parameter_declaration");
constexpr Symbol sym_optional_parameter_declaration =
    Symbol(321, "optional_parameter_declaration");
constexpr Symbol sym_variadic_parameter_declaration =
    Symbol(322, "variadic_parameter_declaration");
constexpr Symbol sym_variadic_declarator = Symbol(323, "variadic_declarator");
constexpr Symbol sym_variadic_reference_declarator =
    Symbol(324, "reference_declarator");
constexpr Symbol sym_operator_cast = Symbol(325, "operator_cast");
constexpr Symbol sym_field_initializer_list =
    Symbol(326, "field_initializer_list");
constexpr Symbol sym_field_initializer = Symbol(327, "field_initializer");
constexpr Symbol sym_inline_method_definition =
    Symbol(328, "function_definition");
constexpr Symbol sym__constructor_specifiers =
    Symbol(329, "_constructor_specifiers");
constexpr Symbol sym_operator_cast_definition =
    Symbol(330, "function_definition");
constexpr Symbol sym_operator_cast_declaration = Symbol(331, "declaration");
constexpr Symbol sym_constructor_try_statement = Symbol(332, "try_statement");
constexpr Symbol sym_constructor_or_destructor_definition =
    Symbol(333, "function_definition");
constexpr Symbol sym_constructor_or_destructor_declaration =
    Symbol(334, "declaration");
constexpr Symbol sym_default_method_clause =
    Symbol(335, "default_method_clause");
constexpr Symbol sym_delete_method_clause = Symbol(336, "delete_method_clause");
constexpr Symbol sym_friend_declaration = Symbol(337, "friend_declaration");
constexpr Symbol sym_access_specifier = Symbol(338, "access_specifier");
constexpr Symbol sym_reference_declarator = Symbol(339, "reference_declarator");
constexpr Symbol sym_reference_field_declarator =
    Symbol(340, "reference_declarator");
constexpr Symbol sym_abstract_reference_declarator =
    Symbol(341, "abstract_reference_declarator");
constexpr Symbol sym_structured_binding_declarator =
    Symbol(342, "structured_binding_declarator");
constexpr Symbol sym_ref_qualifier = Symbol(343, "ref_qualifier");
constexpr Symbol sym__function_declarator_seq =
    Symbol(344, "_function_declarator_seq");
constexpr Symbol sym_trailing_return_type = Symbol(345, "trailing_return_type");
constexpr Symbol sym_noexcept = Symbol(346, "noexcept");
constexpr Symbol sym_throw_specifier = Symbol(347, "throw_specifier");
constexpr Symbol sym_template_type = Symbol(348, "template_type");
constexpr Symbol sym_template_method = Symbol(349, "template_method");
constexpr Symbol sym_template_function = Symbol(350, "template_function");
constexpr Symbol sym_template_argument_list =
    Symbol(351, "template_argument_list");
constexpr Symbol sym_namespace_definition = Symbol(352, "namespace_definition");
constexpr Symbol sym_namespace_alias_definition =
    Symbol(353, "namespace_alias_definition");
constexpr Symbol sym__namespace_specifier = Symbol(354, "_namespace_specifier");
constexpr Symbol sym_nested_namespace_specifier =
    Symbol(355, "nested_namespace_specifier");
constexpr Symbol sym_using_declaration = Symbol(356, "using_declaration");
constexpr Symbol sym_alias_declaration = Symbol(357, "alias_declaration");
constexpr Symbol sym_static_assert_declaration =
    Symbol(358, "static_assert_declaration");
constexpr Symbol sym_concept_definition = Symbol(359, "concept_definition");
constexpr Symbol sym_for_range_loop = Symbol(360, "for_range_loop");
constexpr Symbol sym_init_statement = Symbol(361, "init_statement");
constexpr Symbol sym_condition_clause = Symbol(362, "condition_clause");
constexpr Symbol sym_condition_declaration = Symbol(363, "declaration");
constexpr Symbol sym_co_return_statement = Symbol(364, "co_return_statement");
constexpr Symbol sym_co_yield_statement = Symbol(365, "co_yield_statement");
constexpr Symbol sym_throw_statement = Symbol(366, "throw_statement");
constexpr Symbol sym_try_statement = Symbol(367, "try_statement");
constexpr Symbol sym_catch_clause = Symbol(368, "catch_clause");
constexpr Symbol sym_raw_string_literal = Symbol(369, "raw_string_literal");
constexpr Symbol sym_co_await_expression = Symbol(370, "co_await_expression");
constexpr Symbol sym_new_expression = Symbol(371, "new_expression");
constexpr Symbol sym_new_declarator = Symbol(372, "new_declarator");
constexpr Symbol sym_delete_expression = Symbol(373, "delete_expression");
constexpr Symbol sym_type_requirement = Symbol(374, "type_requirement");
constexpr Symbol sym_compound_requirement = Symbol(375, "compound_requirement");
constexpr Symbol sym__requirement = Symbol(376, "_requirement");
constexpr Symbol sym_requirement_seq = Symbol(377, "requirement_seq");
constexpr Symbol sym_constraint_conjunction =
    Symbol(378, "constraint_conjunction");
constexpr Symbol sym_constraint_disjunction =
    Symbol(379, "constraint_disjunction");
constexpr Symbol sym__requirement_clause_constraint =
    Symbol(380, "_requirement_clause_constraint");
constexpr Symbol sym_requires_clause = Symbol(381, "requires_clause");
constexpr Symbol sym_requires_parameter_list = Symbol(382, "parameter_list");
constexpr Symbol sym_requires_expression = Symbol(383, "requires_expression");
constexpr Symbol sym_lambda_expression = Symbol(384, "lambda_expression");
constexpr Symbol sym_lambda_capture_specifier =
    Symbol(385, "lambda_capture_specifier");
constexpr Symbol sym_lambda_default_capture =
    Symbol(386, "lambda_default_capture");
constexpr Symbol sym__fold_operator = Symbol(387, "_fold_operator");
constexpr Symbol sym__binary_fold_operator =
    Symbol(388, "_binary_fold_operator");
constexpr Symbol sym__unary_left_fold = Symbol(389, "_unary_left_fold");
constexpr Symbol sym__unary_right_fold = Symbol(390, "_unary_right_fold");
constexpr Symbol sym__binary_fold = Symbol(391, "_binary_fold");
constexpr Symbol sym_fold_expression = Symbol(392, "fold_expression");
constexpr Symbol sym_parameter_pack_expansion =
    Symbol(393, "parameter_pack_expansion");
constexpr Symbol sym_type_parameter_pack_expansion =
    Symbol(394, "parameter_pack_expansion");
constexpr Symbol sym_destructor_name = Symbol(395, "destructor_name");
constexpr Symbol sym_dependent_identifier = Symbol(396, "dependent_name");
constexpr Symbol sym_dependent_field_identifier = Symbol(397, "dependent_name");
constexpr Symbol sym_dependent_type_identifier = Symbol(398, "dependent_name");
constexpr Symbol sym__scope_resolution = Symbol(399, "_scope_resolution");
constexpr Symbol sym_qualified_field_identifier =
    Symbol(400, "qualified_identifier");
constexpr Symbol sym_qualified_identifier = Symbol(401, "qualified_identifier");
constexpr Symbol sym_qualified_type_identifier =
    Symbol(402, "qualified_identifier");
constexpr Symbol sym_qualified_operator_cast_identifier =
    Symbol(403, "qualified_identifier");
constexpr Symbol sym_operator_name = Symbol(404, "operator_name");
constexpr Symbol sym_user_defined_literal = Symbol(405, "user_defined_literal");
constexpr Symbol aux_sym_translation_unit_repeat1 =
    Symbol(406, "translation_unit_repeat1");
constexpr Symbol aux_sym_preproc_params_repeat1 =
    Symbol(407, "preproc_params_repeat1");
constexpr Symbol aux_sym_preproc_if_in_field_declaration_list_repeat1 =
    Symbol(408, "preproc_if_in_field_declaration_list_repeat1");
constexpr Symbol aux_sym_preproc_argument_list_repeat1 =
    Symbol(409, "preproc_argument_list_repeat1");
constexpr Symbol aux_sym_declaration_repeat1 =
    Symbol(410, "declaration_repeat1");
constexpr Symbol aux_sym_type_definition_repeat1 =
    Symbol(411, "type_definition_repeat1");
constexpr Symbol aux_sym_type_definition_repeat2 =
    Symbol(412, "type_definition_repeat2");
constexpr Symbol aux_sym__declaration_specifiers_repeat1 =
    Symbol(413, "_declaration_specifiers_repeat1");
constexpr Symbol aux_sym_attribute_declaration_repeat1 =
    Symbol(414, "attribute_declaration_repeat1");
constexpr Symbol aux_sym_attributed_declarator_repeat1 =
    Symbol(415, "attributed_declarator_repeat1");
constexpr Symbol aux_sym_pointer_declarator_repeat1 =
    Symbol(416, "pointer_declarator_repeat1");
constexpr Symbol aux_sym_sized_type_specifier_repeat1 =
    Symbol(417, "sized_type_specifier_repeat1");
constexpr Symbol aux_sym_enumerator_list_repeat1 =
    Symbol(418, "enumerator_list_repeat1");
constexpr Symbol aux_sym_field_declaration_repeat1 =
    Symbol(419, "field_declaration_repeat1");
constexpr Symbol aux_sym_parameter_list_repeat1 =
    Symbol(420, "parameter_list_repeat1");
constexpr Symbol aux_sym_case_statement_repeat1 =
    Symbol(421, "case_statement_repeat1");
constexpr Symbol aux_sym_argument_list_repeat1 =
    Symbol(422, "argument_list_repeat1");
constexpr Symbol aux_sym_initializer_list_repeat1 =
    Symbol(423, "initializer_list_repeat1");
constexpr Symbol aux_sym_initializer_pair_repeat1 =
    Symbol(424, "initializer_pair_repeat1");
constexpr Symbol aux_sym_concatenated_string_repeat1 =
    Symbol(425, "concatenated_string_repeat1");
constexpr Symbol aux_sym_string_literal_repeat1 =
    Symbol(426, "string_literal_repeat1");
constexpr Symbol aux_sym_base_class_clause_repeat1 =
    Symbol(427, "base_class_clause_repeat1");
constexpr Symbol aux_sym_template_parameter_list_repeat1 =
    Symbol(428, "template_parameter_list_repeat1");
constexpr Symbol aux_sym_field_initializer_list_repeat1 =
    Symbol(429, "field_initializer_list_repeat1");
constexpr Symbol aux_sym_operator_cast_definition_repeat1 =
    Symbol(430, "operator_cast_definition_repeat1");
constexpr Symbol aux_sym_constructor_try_statement_repeat1 =
    Symbol(431, "constructor_try_statement_repeat1");
constexpr Symbol aux_sym_structured_binding_declarator_repeat1 =
    Symbol(432, "structured_binding_declarator_repeat1");
constexpr Symbol aux_sym__function_declarator_seq_repeat1 =
    Symbol(433, "_function_declarator_seq_repeat1");
constexpr Symbol aux_sym__function_declarator_seq_repeat2 =
    Symbol(434, "_function_declarator_seq_repeat2");
constexpr Symbol aux_sym_throw_specifier_repeat1 =
    Symbol(435, "throw_specifier_repeat1");
constexpr Symbol aux_sym_template_argument_list_repeat1 =
    Symbol(436, "template_argument_list_repeat1");
constexpr Symbol aux_sym_requirement_seq_repeat1 =
    Symbol(437, "requirement_seq_repeat1");
constexpr Symbol aux_sym_requires_parameter_list_repeat1 =
    Symbol(438, "requires_parameter_list_repeat1");
constexpr Symbol aux_sym_lambda_capture_specifier_repeat1 =
    Symbol(439, "lambda_capture_specifier_repeat1");
constexpr Symbol alias_sym_field_identifier = Symbol(440, "field_identifier");
constexpr Symbol alias_sym_namespace_identifier =
    Symbol(441, "namespace_identifier");
constexpr Symbol alias_sym_simple_requirement =
    Symbol(442, "simple_requirement");
constexpr Symbol alias_sym_statement_identifier =
    Symbol(443, "statement_identifier");
constexpr Symbol alias_sym_type_identifier = Symbol(444, "type_identifier");
}  // namespace symbols

namespace fields {
constexpr Field field_alternative = Field(1, "alternative");
constexpr Field field_argument = Field(2, "argument");
constexpr Field field_arguments = Field(3, "arguments");
constexpr Field field_base = Field(4, "base");
constexpr Field field_body = Field(5, "body");
constexpr Field field_captures = Field(6, "captures");
constexpr Field field_condition = Field(7, "condition");
constexpr Field field_consequence = Field(8, "consequence");
constexpr Field field_constraint = Field(9, "constraint");
constexpr Field field_declarator = Field(10, "declarator");
constexpr Field field_default_type = Field(11, "default_type");
constexpr Field field_default_value = Field(12, "default_value");
constexpr Field field_delimiter = Field(13, "delimiter");
constexpr Field field_designator = Field(14, "designator");
constexpr Field field_directive = Field(15, "directive");
constexpr Field field_field = Field(16, "field");
constexpr Field field_function = Field(17, "function");
constexpr Field field_index = Field(18, "index");
constexpr Field field_initializer = Field(19, "initializer");
constexpr Field field_label = Field(20, "label");
constexpr Field field_left = Field(21, "left");
constexpr Field field_length = Field(22, "length");
constexpr Field field_message = Field(23, "message");
constexpr Field field_name = Field(24, "name");
constexpr Field field_operator = Field(25, "operator");
constexpr Field field_parameters = Field(26, "parameters");
constexpr Field field_path = Field(27, "path");
constexpr Field field_pattern = Field(28, "pattern");
constexpr Field field_placement = Field(29, "placement");
constexpr Field field_prefix = Field(30, "prefix");
constexpr Field field_requirements = Field(31, "requirements");
constexpr Field field_right = Field(32, "right");
constexpr Field field_scope = Field(33, "scope");
constexpr Field field_size = Field(34, "size");
constexpr Field field_template_parameters = Field(35, "template_parameters");
constexpr Field field_type = Field(36, "type");
constexpr Field field_update = Field(37, "update");
constexpr Field field_value = Field(38, "value");
}  // namespace fields

}  // namespace cpp

}  // namespace ts
}  // namespace mergebot

#endif  // MB_INCLUDE_MERGEBOT_PARSER_LANGUAGES_CPP_H

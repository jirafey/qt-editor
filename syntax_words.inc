// ==== 1. STRICT C++ KEYWORDS (Up to C++26 Draft) ====
"alignas", "alignof", "and", "and_eq", "asm", "atomic_cancel", "atomic_commit", 
"atomic_noexcept", "auto", "bitand", "bitor", "bool", "break", "case", "catch", 
"char", "char8_t", "char16_t", "char32_t", "class", "compl", "concept", "const", 
"consteval", "constexpr", "constinit", "const_cast", "continue", "co_await", 
"co_return", "co_yield", "decltype", "default", "delete", "do", "double", 
"dynamic_cast", "else", "enum", "explicit", "export", "extern", "false", "float", 
"for", "friend", "goto", "if", "inline", "int", "long", "mutable", "namespace", 
"new", "noexcept", "not", "not_eq", "nullptr", "operator", "or", "or_eq", 
"private", "protected", "public", "reflexpr", "register", "reinterpret_cast", 
"requires", "return", "short", "signed", "sizeof", "static", "static_assert", 
"static_cast", "struct", "switch", "synchronized", "template", "this", 
"thread_local", "throw", "true", "try", "typedef", "typeid", "typename", "union", 
"unsigned", "using", "virtual", "void", "volatile", "wchar_t", "while", "xor", "xor_eq",

// ==== 2. C++20 MODULES & C++26 CONTRACTS ====
"import", "module", "pre", "post", "contract_assert",

// ==== 3. STRICT C KEYWORDS (C99 / C11 / C23) ====
"restrict", "_Alignas", "_Alignof", "_Atomic", "_Bool", "_Complex", "_Generic", 
"_Imaginary", "_Noreturn", "_Static_assert", "_Thread_local", 
"typeof", "typeof_unqual", "_BitInt", "_Decimal32", "_Decimal64", "_Decimal128",

// ==== 4. PREPROCESSOR BUILT-INS ====
"defined", "__has_include", "__has_embed", "__has_cpp_attribute", 
"__FILE__", "__LINE__", "__func__", "__DATE__", "__TIME__", "_Pragma",

// ==== 5. COMMON STANDARD LIBRARY TYPES ====
"std", "size_t", "ptrdiff_t", "int8_t", "int16_t", "int32_t", "int64_t", 
"uint8_t", "uint16_t", "uint32_t", "uint64_t", "string", "vector", "map", 
"set", "list", "array", "deque", "unordered_map", "unordered_set", 
"shared_ptr", "unique_ptr", "weak_ptr",

// ==== 6. QT FRAMEWORK KEYWORDS & TYPES ====
"signals", "slots", "emit", "Q_OBJECT", "Q_PROPERTY", "Q_INVOKABLE", 
"Q_ENUM", "Q_FLAG", "Q_SIGNALS", "Q_SLOTS", "Q_DECLARE_METATYPE",
"QString", "QList", "QMap", "QVector", "QObject", "QWidget", "QByteArray", 
"QVariant", "QHash", "QSet", "QAction", "QMainWindow", "QDialog"

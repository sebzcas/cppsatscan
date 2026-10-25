#ifndef _Str_H_
#define _Str_H_

#include "base.h"

class str {
public:
	str(const char* s = "");
	str(const int&);
    str(const double&);
	str(void*);

    str(const str&);
	str& operator = (const str &);
	// TODO: str& str::operator=(const str&&) noexcept;
	
	bool is_int();
	bool is_double();
	bool is_num();
	bool is_alpha();
	bool contains_alpha();

	int to_int();
	double to_double();
	const char* to_string();
	void* to_ptr();

	str lsubstr(const int& idx = 1);
	str rsubstr(const int& idx = 1);

	friend std::ostream& operator << (std::ostream&, const str&);
	friend str operator + (const str&, const str&);
	friend str operator + (const str&, const char*);
	friend str operator + (const char*, const str&);
	friend bool operator == (const str&, const str&);
	friend bool operator == (const str&, const char*);
	friend bool operator == (const char*, const str&);
	friend str operator += (const str&, const str&);

	~str();

    const size_t size();

private:
	char* c_val;
    unsigned int ct_size;
};

#include "str.cpp"
#endif

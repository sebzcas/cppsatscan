#include "str.h"


str::str(const str& s) {
    c_val = s.c_val;
    ct_size = s.ct_size;
}

str& str::operator = (const str &s) {
    c_val = s.c_val;
    ct_size = s.ct_size;
    return *this;
}

str::str(const char* s) {
    c_val = const_cast<char*>(s);
    ct_size = (s == "") ? 1 : get_size(s) + 1;
}

str::str(const int& v) {
	int a = v;
    int cnt = 0;

    int x = a;
	while (x) {
		x /= 10;
		cnt++;
	}
    if (v <= 0)
        cnt++;

	char* val = new char[cnt + 1];
    val[cnt] = '\0';
    ct_size = cnt + 1;
    cnt--;

    int min_c = 0;
	if (v < 0) {
		*(val) = '-';
        min_c = 1;
        a = -a;
	}
    
    int b;
	while (cnt >= min_c) {
        b = a;
        a /= 10;
        *(val + cnt) = (b - a * 10) + '0';
        cnt--;
	}

    c_val = val;
    val = nullptr;
}

str::str(const double& v) {
    double d = v;
    int cnt = 0;

    long long x = (long long) d;
    double dlt = d - x;
	while (dlt > 1e-10 || dlt < -1e-10) {
        d *= 10;
        cnt++;
        dlt = d - (long long) d;
    }
    auto pos = cnt;

	while (x) {
		x /= 10;
		cnt++;
	}
    bool add_zero = pos == 0;
    pos = cnt - pos;
    cnt++;
    if (add_zero) cnt++;
    if (v < 0) cnt++;
    if (v == 0) cnt++;
    if (-1 < v && v < 1 && v != 0) cnt++;

	char* val = new char[cnt + 1];

    val[cnt] = '\0';
    ct_size = cnt + 1;
    cnt--;
    if (add_zero) {
        val[cnt] = '0';
        cnt--;
        pos = cnt;
    }

    long long b;
    int min_c = 0;
    auto c = (long long) d;
    if (v < 0) {
        val[0] = '-';
        min_c = 1;
        c = -c;
        if (!add_zero)
            pos++;
    }
    if (-1 < v && v < 1 && v != 0) {
        val[1] = '0';
        pos++;
    }

	while (cnt >= min_c) {
        if (cnt == pos) {
            *(val + cnt) = '.';
        } else {
            b = c;
            c /= 10;
            *(val + cnt) = (b - c * 10) + '0';
        }
        cnt--;
	}

    c_val = val;
    val = nullptr;
}

str::str(void* p) {
    int p_int = (uintptr_t) p;
    str s(p_int);
    c_val = s.c_val;
    ct_size = s.ct_size;
}

str::~str() {
    // do nothing here for parameter `c_val` since `const char*` is a pointer points to a constant
    // and its memory will be free by compiler automatically after it exits its scope
}

bool str::is_int() {
    if (c_val[0] != '-' && (c_val[0] < '0' || c_val[0] > '9'))
        return false;

    for (unsigned int i = 1; i < ct_size - 1; i++) {
        if (c_val[i] < '0' || c_val[i] > '9')
            return false;
    }

    return true;
}

bool str::is_double() {
    if (c_val[0] != '-' && (c_val[0] < '0' || c_val[0] > '9'))
        return false;

    unsigned int d = 0;

    for (unsigned int i = 1; i < ct_size - 1; i++) {
        if (c_val[i] == '.' && i < ct_size - 2) {
            d++;
            continue;
        }

        if (c_val[i] < '0' || c_val[i] > '9')
            return false;
    }

    if (d != 1)
        return false;

    return true;
}

bool str::is_num() {
    return is_int() || is_double();
}

bool str::is_alpha() {
    for (unsigned int i = 0; i < ct_size - 1; i++) {
        if (!(('a' <= c_val[i] && c_val[i] <= 'z') || ('A' <= c_val[i] && c_val[i] <= 'Z')))
            return false;
    }
    return true;
}

bool str::contains_alpha() {
    for (unsigned int i = 0; i < ct_size - 1; i++) {
        if ('a' <= c_val[i] && c_val[i] <= 'z')
            return true;
        if ('A' <= c_val[i] && c_val[i] <= 'Z')
            return true;
    }
    return false;
}

int str::to_int() {
    int res = 0;
    int b = 1;

    unsigned int i = ct_size - 2;
    while (i) {
        res += (*(c_val + i) - 48) * b;
        b *= 10;
        i--;
    }

    char c = *(c_val + i);
    if (c == '-') {
        res = -res;
    } else {
        res += (c - 48) * b;
    }

    return res;
}

double str::to_double() {
    double res = 0.0;

    unsigned int i = ct_size - 2;
    unsigned int d = 0;
    char c;
    while (i) {
        res *= 0.1;

        c = c_val[i];
        if (c == '.')
            break;
        
        res += (c - 48);
        i--;
    }
    i--;

    int b = 1;
    while (i) {
        res += (c_val[i] - 48) * b;
        b *= 10;
        i--;
    }

    c = c_val[i];
    if (c == '-') {
        res = -res;
    } else {
        res += (c - 48) * b;
    }

    return res;
}

const char* str::to_string() {
    return const_cast<char*>(c_val);
}

void* str::to_ptr() {
    int p_int = to_int();
    return reinterpret_cast<void*>(p_int);
}

const size_t str::size() {
    return ct_size - 1;
}

str str::lsubstr(const int& idx) {
    if (idx < 0)
        throw "parameter `idx` cannot be negative";

    if (idx >= ct_size)
        return str(const_cast<char*>(c_val));

    char *s = new char[idx + 1];
    for (unsigned int i = 0; i < idx; ++i)
        s[i] = c_val[i];
    s[idx] = '\0';
    
    return str(const_cast<char*>(s));
}

str str::rsubstr(const int& idx) {
    if (idx < 0)
        throw "parameter `idx` cannot be negative";

    if (idx >= ct_size)
        return str(const_cast<char*>(c_val));

    char *s = new char[idx + 1];
    for (unsigned int i = ct_size - idx - 1, j = 0; i < ct_size; ++i, j++)
        s[j] = c_val[i];
    s[idx] = '\0';
    
    return str(const_cast<char*>(s));
}

ostream& operator << (ostream& out, const str& s) {
	cout << "\"" << s.c_val << "\"";
	return out;
}

str operator + (const str& s1, const str& s2) {
    char *val = new char[s1.ct_size + s2.ct_size - 1];
    
    unsigned int i = 0;
    while (i < s1.ct_size - 1) {
        *(val + i) = *(s1.c_val + i);
        i++;
    }
    
    unsigned int j = 0;
    while (j < s2.ct_size - 1) {
        *(val + i) = *(s2.c_val + j);
        j++;
        i++;
    }
    
    *(val + i) = '\0';
    return str(val);
}

str operator + (const str& s, const char* cs) {
    char *val = new char[s.ct_size + get_size(cs)];
    
    unsigned int i = 0;
    while (i < s.ct_size - 1) {
        *(val + i) = *(s.c_val + i);
        i++;
    }
    
    unsigned int j = 0;
    while (j < get_size(cs)) {
        *(val + i) = *(cs + j);
        j++;
        i++;
    }
    
    *(val + i) = '\0';
    return str(val);
}

str operator + (const char* cs, const str& s) {
    char *val = new char[s.ct_size + get_size(cs)];
    
    unsigned int i = 0;
    while (i < get_size(cs)) {
        *(val + i) = *(cs + i);
        i++;
    }
    
    unsigned int j = 0;
    while (j < s.ct_size - 1) {
        *(val + i) = *(s.c_val + j);
        j++;
        i++;
    }
    
    *(val + i) = '\0';
    return str(val);
}

bool operator == (const str& sl, const str& sr) {
    return compare(const_cast<char*>(sl.c_val), const_cast<char*>(sr.c_val)) == 0;
}

bool operator == (const str& s, const char* cs) {
    return compare(const_cast<char*>(s.c_val), cs) == 0;
}

bool operator == (const char* cs, const str& s) {
    return compare(const_cast<char*>(s.c_val), cs) == 0;
}

str operator += (str& sl, const str& sr) {
    sl = sl + sr;
    return sl;
}

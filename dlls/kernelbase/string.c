/*
 * Copyright 2019 Nikolay Sivov for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "shlwapi.h"
#include "winternl.h"

#include "wine/debug.h"
#include "wine/exception.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(string);

static WORD get_char_type(WCHAR ch)
{
    WORD type = 0;
    GetStringTypeW(CT_CTYPE1, &ch, 1, &type);
    return type;
}

static BOOL char_compare(WORD ch1, WORD ch2, DWORD flags)
{
    char str1[3], str2[3];

    str1[0] = LOBYTE(ch1);
    if (IsDBCSLeadByte(str1[0]))
    {
        str1[1] = HIBYTE(ch1);
        str1[2] = '\0';
    }
    else
        str1[1] = '\0';

    str2[0] = LOBYTE(ch2);
    if (IsDBCSLeadByte(str2[0]))
    {
        str2[1] = HIBYTE(ch2);
        str2[2] = '\0';
    }
    else
        str2[1] = '\0';

    return CompareStringA(GetThreadLocale(), flags, str1, -1, str2, -1) - CSTR_EQUAL;
}

DWORD WINAPI StrCmpCA(const char *str, const char *cmp)
{
    return lstrcmpA(str, cmp);
}

DWORD WINAPI StrCmpCW(const WCHAR *str, const WCHAR *cmp)
{
    return lstrcmpW(str, cmp);
}

DWORD WINAPI StrCmpICA(const char *str, const char *cmp)
{
    return lstrcmpiA(str, cmp);
}

DWORD WINAPI StrCmpICW(const WCHAR *str, const WCHAR *cmp)
{
    return lstrcmpiW(str, cmp);
}

DWORD WINAPI StrCmpNICA(const char *str, const char *cmp, DWORD len)
{
    return StrCmpNIA(str, cmp, len);
}

DWORD WINAPI StrCmpNICW(const WCHAR *str, const WCHAR *cmp, DWORD len)
{
    return StrCmpNIW(str, cmp, len);
}

BOOL WINAPI IsCharBlankW(WCHAR wc)
{
    return !!(get_char_type(wc) & C1_BLANK);
}

BOOL WINAPI IsCharCntrlW(WCHAR wc)
{
    return !!(get_char_type(wc) & C1_CNTRL);
}

BOOL WINAPI IsCharDigitW(WCHAR wc)
{
    return !!(get_char_type(wc) & C1_DIGIT);
}

BOOL WINAPI IsCharPunctW(WCHAR wc)
{
    return !!(get_char_type(wc) & C1_PUNCT);
}

BOOL WINAPI IsCharSpaceA(CHAR c)
{
    WORD type;
    return GetStringTypeA(GetSystemDefaultLCID(), CT_CTYPE1, &c, 1, &type) && (type & C1_SPACE);
}

BOOL WINAPI IsCharSpaceW(WCHAR wc)
{
    return !!(get_char_type(wc) & C1_SPACE);
}

BOOL WINAPI IsCharXDigitW(WCHAR wc)
{
    return !!(get_char_type(wc) & C1_XDIGIT);
}

BOOL WINAPI IsCharAlphaA(CHAR x)
{
    WCHAR wch;
    MultiByteToWideChar(CP_ACP, 0, &x, 1, &wch, 1);
    return IsCharAlphaW(wch);
}

BOOL WINAPI IsCharAlphaW(WCHAR ch)
{
    return !!(get_char_type(ch) & C1_ALPHA);
}

BOOL WINAPI IsCharLowerA(CHAR x)
{
    WCHAR wch;
    MultiByteToWideChar(CP_ACP, 0, &x, 1, &wch, 1);
    return IsCharLowerW(wch);
}

BOOL WINAPI IsCharLowerW(WCHAR ch)
{
    return !!(get_char_type(ch) & C1_LOWER);
}

BOOL WINAPI IsCharAlphaNumericA(CHAR x)
{
    WCHAR wch;
    MultiByteToWideChar(CP_ACP, 0, &x, 1, &wch, 1);
    return IsCharAlphaNumericW(wch);
}

BOOL WINAPI IsCharAlphaNumericW(WCHAR ch)
{
    return !!(get_char_type(ch) & (C1_ALPHA | C1_DIGIT));
}

BOOL WINAPI IsCharUpperA(CHAR x)
{
    WCHAR wch;
    MultiByteToWideChar(CP_ACP, 0, &x, 1, &wch, 1);
    return IsCharUpperW(wch);
}

BOOL WINAPI IsCharUpperW(WCHAR ch)
{
    return !!(get_char_type(ch) & C1_UPPER);
}

char * WINAPI StrChrA(const char *str, WORD ch)
{
    TRACE("%s, %#x\n", wine_dbgstr_a(str), ch);

    if (!str)
        return NULL;

    while (*str)
    {
        if (!char_compare(*str, ch, 0))
            return (char *)str;
        str = CharNextA(str);
    }

    return NULL;
}

WCHAR * WINAPI StrChrW(const WCHAR *str, WCHAR ch)
{
    TRACE("%s, %#x\n", wine_dbgstr_w(str), ch);

    if (!str)
        return NULL;

    return strchrW(str, ch);
}

char * WINAPI StrChrIA(const char *str, WORD ch)
{
    TRACE("%s, %i\n", wine_dbgstr_a(str), ch);

    if (!str)
        return NULL;

    while (*str)
    {
        if (!ChrCmpIA(*str, ch))
            return (char *)str;
        str = CharNextA(str);
    }

    return NULL;
}

WCHAR * WINAPI StrChrIW(const WCHAR *str, WCHAR ch)
{
    TRACE("%s, %#x\n", wine_dbgstr_w(str), ch);

    if (!str)
        return NULL;

    ch = toupperW(ch);
    while (*str)
    {
        if (toupperW(*str) == ch)
            return (WCHAR *)str;
        str++;
    }
    str = NULL;

    return (WCHAR *)str;
}

WCHAR * WINAPI StrChrNW(const WCHAR *str, WCHAR ch, UINT max_len)
{
    TRACE("%s, %#x, %u\n", wine_dbgstr_wn(str, max_len), ch, max_len);

    if (!str)
        return NULL;

    while (*str && max_len-- > 0)
    {
        if (*str == ch)
            return (WCHAR *)str;
        str++;
    }

    return NULL;
}

char * WINAPI StrDupA(const char *str)
{
    unsigned int len;
    char *ret;

    TRACE("%s\n", wine_dbgstr_a(str));

    len = str ? strlen(str) + 1 : 1;
    ret = LocalAlloc(LMEM_FIXED, len);

    if (ret)
    {
        if (str)
            memcpy(ret, str, len);
        else
            *ret = '\0';
    }

    return ret;
}

WCHAR * WINAPI StrDupW(const WCHAR *str)
{
    unsigned int len;
    WCHAR *ret;

    TRACE("%s\n", wine_dbgstr_w(str));

    len = (str ? strlenW(str) + 1 : 1) * sizeof(WCHAR);
    ret = LocalAlloc(LMEM_FIXED, len);

    if (ret)
    {
        if (str)
            memcpy(ret, str, len);
        else
            *ret = '\0';
    }

    return ret;
}

BOOL WINAPI ChrCmpIA(WORD ch1, WORD ch2)
{
    TRACE("%#x, %#x\n", ch1, ch2);

    return char_compare(ch1, ch2, NORM_IGNORECASE);
}

BOOL WINAPI ChrCmpIW(WCHAR ch1, WCHAR ch2)
{
    return CompareStringW(GetThreadLocale(), NORM_IGNORECASE, &ch1, 1, &ch2, 1) - CSTR_EQUAL;
}

WCHAR * WINAPI StrStrW(const WCHAR *str, const WCHAR *search)
{
    TRACE("%s, %s\n", wine_dbgstr_w(str), wine_dbgstr_w(search));

    if (!str || !search || !*search)
        return NULL;

    return strstrW(str, search);
}

WCHAR * WINAPI StrStrNW(const WCHAR *str, const WCHAR *search, UINT max_len)
{
    unsigned int i, len;

    TRACE("%s, %s, %u\n", wine_dbgstr_w(str), wine_dbgstr_w(search), max_len);

    if (!str || !search || !*search || !max_len)
        return NULL;

    len = strlenW(search);

    for (i = max_len; *str && (i > 0); i--, str++)
    {
        if (!strncmpW(str, search, len))
            return (WCHAR *)str;
    }

    return NULL;
}

int WINAPI StrCmpNIA(const char *str, const char *cmp, int len)
{
    TRACE("%s, %s, %i\n", wine_dbgstr_a(str), wine_dbgstr_a(cmp), len);
    return CompareStringA(GetThreadLocale(), NORM_IGNORECASE, str, len, cmp, len) - CSTR_EQUAL;
}

WCHAR * WINAPI StrStrNIW(const WCHAR *str, const WCHAR *search, UINT max_len)
{
    unsigned int i, len;

    TRACE("%s, %s, %u\n", wine_dbgstr_w(str), wine_dbgstr_w(search), max_len);

    if (!str || !search || !*search || !max_len)
        return NULL;

    len = strlenW(search);

    for (i = max_len; *str && (i > 0); i--, str++)
    {
        if (!strncmpiW(str, search, len))
            return (WCHAR *)str;
    }

    return NULL;
}

int WINAPI StrCmpNA(const char *str, const char *comp, int len)
{
    TRACE("%s, %s, %i\n", wine_dbgstr_a(str), wine_dbgstr_a(comp), len);
    return CompareStringA(GetThreadLocale(), 0, str, len, comp, len) - CSTR_EQUAL;
}

int WINAPI StrCmpNW(const WCHAR *str, const WCHAR *comp, int len)
{
    TRACE("%s, %s, %i\n", wine_dbgstr_w(str), wine_dbgstr_w(comp), len);
    return CompareStringW(GetThreadLocale(), 0, str, len, comp, len) - CSTR_EQUAL;
}

int WINAPI StrCmpNIW(const WCHAR *str, const WCHAR *comp, int len)
{
    TRACE("%s, %s, %i\n", wine_dbgstr_w(str), wine_dbgstr_w(comp), len);
    return CompareStringW(GetThreadLocale(), NORM_IGNORECASE, str, len, comp, len) - CSTR_EQUAL;
}

int WINAPI StrCmpW(const WCHAR *str, const WCHAR *comp)
{
    TRACE("%s, %s\n", wine_dbgstr_w(str), wine_dbgstr_w(comp));
    return CompareStringW(GetThreadLocale(), 0, str, -1, comp, -1) - CSTR_EQUAL;
}

int WINAPI StrCmpIW(const WCHAR *str, const WCHAR *comp)
{
    TRACE("%s, %s\n", wine_dbgstr_w(str), wine_dbgstr_w(comp));
    return CompareStringW(GetThreadLocale(), NORM_IGNORECASE, str, -1, comp, -1) - CSTR_EQUAL;
}

WCHAR * WINAPI StrCpyNW(WCHAR *dst, const WCHAR *src, int count)
{
    const WCHAR *s = src;
    WCHAR *d = dst;

    TRACE("%p, %s, %i\n", dst, wine_dbgstr_w(src), count);

    if (s)
    {
        while ((count > 1) && *s)
        {
            count--;
            *d++ = *s++;
        }
    }
    if (count) *d = 0;

    return dst;
}

WCHAR * WINAPI StrStrIW(const WCHAR *str, const WCHAR *search)
{
    unsigned int len;
    const WCHAR *end;

    TRACE("%s, %s\n", wine_dbgstr_w(str), wine_dbgstr_w(search));

    if (!str || !search || !*search)
        return NULL;

    len = strlenW(search);
    end = str + strlenW(str);

    while (str + len <= end)
    {
        if (!StrCmpNIW(str, search, len))
            return (WCHAR *)str;
        str++;
    }

    return NULL;
}

int WINAPI StrSpnW(const WCHAR *str, const WCHAR *match)
{
    if (!str || !match) return 0;
    return strspnW(str, match);
}

WCHAR * WINAPI StrRChrW(const WCHAR *str, const WCHAR *end, WORD ch)
{
    WCHAR *ret = NULL;

    if (!str) return NULL;
    if (!end) end = str + strlenW(str);
    while (str < end)
    {
        if (*str == ch) ret = (WCHAR *)str;
        str++;
    }
    return ret;
}

WCHAR * WINAPI StrRChrIW(const WCHAR *str, const WCHAR *end, WORD ch)
{
    WCHAR *ret = NULL;

    if (!str) return NULL;
    if (!end) end = str + strlenW(str);
    while (str < end)
    {
        if (!ChrCmpIW(*str, ch)) ret = (WCHAR *)str;
        str++;
    }
    return ret;
}

char * WINAPI StrPBrkA(const char *str, const char *match)
{
    TRACE("%s, %s\n", wine_dbgstr_a(str), wine_dbgstr_a(match));

    if (!str || !match || !*match)
        return NULL;

    while (*str)
    {
        if (StrChrA(match, *str))
            return (char *)str;
        str = CharNextA(str);
    }

    return NULL;
}

WCHAR * WINAPI StrPBrkW(const WCHAR *str, const WCHAR *match)
{
    if (!str || !match) return NULL;
    return strpbrkW(str, match);
}

BOOL WINAPI StrTrimA(char *str, const char *trim)
{
    unsigned int len;
    BOOL ret = FALSE;
    char *ptr = str;

    TRACE("%s, %s\n", debugstr_a(str), debugstr_a(trim));

    if (!str || !*str)
        return FALSE;

    while (*ptr && StrChrA(trim, *ptr))
        ptr = CharNextA(ptr); /* Skip leading matches */

    len = strlen(ptr);

    if (ptr != str)
    {
        memmove(str, ptr, len + 1);
        ret = TRUE;
    }

    if (len > 0)
    {
        ptr = str + len;
        while (StrChrA(trim, ptr[-1]))
            ptr = CharPrevA(str, ptr); /* Skip trailing matches */

        if (ptr != str + len)
        {
            *ptr = '\0';
            ret = TRUE;
        }
    }

    return ret;
}

BOOL WINAPI StrTrimW(WCHAR *str, const WCHAR *trim)
{
    unsigned int len;
    WCHAR *ptr = str;
    BOOL ret = FALSE;

    TRACE("%s, %s\n", wine_dbgstr_w(str), wine_dbgstr_w(trim));

    if (!str || !*str)
        return FALSE;

    while (*ptr && StrChrW(trim, *ptr))
        ptr++;

    len = strlenW(ptr);

    if (ptr != str)
    {
        memmove(str, ptr, (len + 1) * sizeof(WCHAR));
        ret = TRUE;
    }

    if (len > 0)
    {
        ptr = str + len;
        while (StrChrW(trim, ptr[-1]))
            ptr--; /* Skip trailing matches */

        if (ptr != str + len)
        {
            *ptr = '\0';
            ret = TRUE;
        }
    }

    return ret;
}

BOOL WINAPI StrToInt64ExA(const char *str, DWORD flags, LONGLONG *ret)
{
    BOOL negative = FALSE;
    LONGLONG value = 0;

    TRACE("%s, %#x, %p\n", wine_dbgstr_a(str), flags, ret);

    if (!str || !ret)
        return FALSE;

    if (flags > STIF_SUPPORT_HEX)
        WARN("Unknown flags %#x\n", flags);

    /* Skip leading space, '+', '-' */
    while (isspace(*str))
        str = CharNextA(str);

    if (*str == '-')
    {
        negative = TRUE;
        str++;
    }
    else if (*str == '+')
        str++;

    if (flags & STIF_SUPPORT_HEX && *str == '0' && tolower(str[1]) == 'x')
    {
        /* Read hex number */
        str += 2;

        if (!isxdigit(*str))
            return FALSE;

        while (isxdigit(*str))
        {
            value *= 16;
            if (isdigit(*str))
                value += (*str - '0');
            else
                value += 10 + (tolower(*str) - 'a');
            str++;
        }

        *ret = value;
        return TRUE;
    }

    /* Read decimal number */
    if (!isdigit(*str))
        return FALSE;

    while (isdigit(*str))
    {
        value *= 10;
        value += (*str - '0');
        str++;
    }

    *ret = negative ? -value : value;
    return TRUE;
}

BOOL WINAPI StrToInt64ExW(const WCHAR *str, DWORD flags, LONGLONG *ret)
{
    BOOL negative = FALSE;
    LONGLONG value = 0;

    TRACE("%s, %#x, %p\n", wine_dbgstr_w(str), flags, ret);

    if (!str || !ret)
        return FALSE;

    if (flags > STIF_SUPPORT_HEX)
        WARN("Unknown flags %#x.\n", flags);

    /* Skip leading space, '+', '-' */
    while (isspaceW(*str))
        str++;

    if (*str == '-')
    {
        negative = TRUE;
        str++;
    }
    else if (*str == '+')
        str++;

    if (flags & STIF_SUPPORT_HEX && *str == '0' && tolowerW(str[1]) == 'x')
    {
        /* Read hex number */
        str += 2;

        if (!isxdigitW(*str))
            return FALSE;

        while (isxdigitW(*str))
        {
            value *= 16;
            if (isdigitW(*str))
                value += (*str - '0');
            else
                value += 10 + (tolowerW(*str) - 'a');
            str++;
        }

        *ret = value;
        return TRUE;
    }

    /* Read decimal number */
    if (!isdigitW(*str))
        return FALSE;

    while (isdigitW(*str))
    {
        value *= 10;
        value += (*str - '0');
        str++;
    }

    *ret = negative ? -value : value;
    return TRUE;
}

BOOL WINAPI StrToIntExA(const char *str, DWORD flags, INT *ret)
{
    LONGLONG value;
    BOOL res;

    TRACE("%s, %#x, %p\n", wine_dbgstr_a(str), flags, ret);

    res = StrToInt64ExA(str, flags, &value);
    if (res) *ret = value;
    return res;
}

BOOL WINAPI StrToIntExW(const WCHAR *str, DWORD flags, INT *ret)
{
    LONGLONG value;
    BOOL res;

    TRACE("%s, %#x, %p\n", wine_dbgstr_w(str), flags, ret);

    res = StrToInt64ExW(str, flags, &value);
    if (res) *ret = value;
    return res;
}

int WINAPI StrToIntA(const char *str)
{
    int value = 0;

    TRACE("%s\n", wine_dbgstr_a(str));

    if (!str)
        return 0;

    if (*str == '-' || isdigit(*str))
        StrToIntExA(str, 0, &value);

    return value;
}

int WINAPI StrToIntW(const WCHAR *str)
{
    int value = 0;

    TRACE("%s\n", wine_dbgstr_w(str));

    if (!str)
        return 0;

    if (*str == '-' || isdigitW(*str))
        StrToIntExW(str, 0, &value);
    return value;
}

char * WINAPI StrCpyNXA(char *dst, const char *src, int len)
{
    TRACE("%p, %s, %i\n", dst, wine_dbgstr_a(src), len);

    if (dst && src && len > 0)
    {
        while ((len-- > 1) && *src)
            *dst++ = *src++;
        if (len >= 0)
            *dst = '\0';
    }

    return dst;
}

WCHAR * WINAPI StrCpyNXW(WCHAR *dst, const WCHAR *src, int len)
{
    TRACE("%p, %s, %i\n", dst, wine_dbgstr_w(src), len);

    if (dst && src && len > 0)
    {
        while ((len-- > 1) && *src)
            *dst++ = *src++;
        if (len >= 0)
            *dst = '\0';
    }

    return dst;
}

LPSTR WINAPI CharLowerA(char *str)
{
    if (IS_INTRESOURCE(str))
    {
        char ch = LOWORD(str);
        CharLowerBuffA( &ch, 1 );
        return (LPSTR)(UINT_PTR)(BYTE)ch;
    }

    __TRY
    {
        CharLowerBuffA( str, strlen(str) );
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return NULL;
    }
    __ENDTRY
    return str;
}

DWORD WINAPI CharLowerBuffA(char *str, DWORD len)
{
    DWORD lenW;
    WCHAR buffer[32];
    WCHAR *strW = buffer;

    if (!str) return 0; /* YES */

    lenW = MultiByteToWideChar(CP_ACP, 0, str, len, NULL, 0);
    if (lenW > ARRAY_SIZE(buffer))
    {
        strW = HeapAlloc(GetProcessHeap(), 0, lenW * sizeof(WCHAR));
        if (!strW) return 0;
    }
    MultiByteToWideChar(CP_ACP, 0, str, len, strW, lenW);
    CharLowerBuffW(strW, lenW);
    len = WideCharToMultiByte(CP_ACP, 0, strW, lenW, str, len, NULL, NULL);
    if (strW != buffer) HeapFree(GetProcessHeap(), 0, strW);
    return len;
}

DWORD WINAPI CharLowerBuffW(WCHAR *str, DWORD len)
{
    if (!str) return 0; /* YES */
    return LCMapStringW(LOCALE_USER_DEFAULT, LCMAP_LOWERCASE, str, len, str, len);
}

LPWSTR WINAPI CharLowerW(WCHAR *str)
{
    if (!IS_INTRESOURCE(str))
    {
        CharLowerBuffW(str, lstrlenW(str));
        return str;
    }
    else
    {
        WCHAR ch = LOWORD(str);
        CharLowerBuffW(&ch, 1);
        return (LPWSTR)(UINT_PTR)ch;
    }
}

LPSTR WINAPI CharNextA(const char *ptr)
{
    if (!*ptr) return (LPSTR)ptr;
    if (IsDBCSLeadByte( ptr[0] ) && ptr[1]) return (LPSTR)(ptr + 2);
    return (LPSTR)(ptr + 1);
}

LPSTR WINAPI CharNextExA(WORD codepage, const char *ptr, DWORD flags)
{
    if (!*ptr) return (LPSTR)ptr;
    if (IsDBCSLeadByteEx( codepage, ptr[0] ) && ptr[1]) return (LPSTR)(ptr + 2);
    return (LPSTR)(ptr + 1);
}

LPWSTR WINAPI CharNextW(const WCHAR *x)
{
    if (*x) x++;

    return (WCHAR *)x;
}

LPSTR WINAPI CharPrevA(const char *start, const char *ptr)
{
    while (*start && (start < ptr))
    {
        LPCSTR next = CharNextA(start);
        if (next >= ptr) break;
        start = next;
    }
    return (LPSTR)start;
}

LPSTR WINAPI CharPrevExA(WORD codepage, const char *start, const char *ptr, DWORD flags)
{
    while (*start && (start < ptr))
    {
        LPCSTR next = CharNextExA(codepage, start, flags);
        if (next >= ptr) break;
        start = next;
    }
    return (LPSTR)start;
}

LPWSTR WINAPI CharPrevW(const WCHAR *start, const WCHAR *x)
{
    if (x > start) return (LPWSTR)(x - 1);
    else return (LPWSTR)x;
}

LPSTR WINAPI CharUpperA(LPSTR str)
{
    if (IS_INTRESOURCE(str))
    {
        char ch = LOWORD(str);
        CharUpperBuffA(&ch, 1);
        return (LPSTR)(UINT_PTR)(BYTE)ch;
    }

    __TRY
    {
        CharUpperBuffA(str, strlen(str));
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }
    __ENDTRY
    return str;
}

DWORD WINAPI CharUpperBuffA(LPSTR str, DWORD len)
{
    DWORD lenW;
    WCHAR buffer[32];
    WCHAR *strW = buffer;

    if (!str) return 0; /* YES */

    lenW = MultiByteToWideChar(CP_ACP, 0, str, len, NULL, 0);
    if (lenW > ARRAY_SIZE(buffer))
    {
        strW = HeapAlloc(GetProcessHeap(), 0, lenW * sizeof(WCHAR));
        if (!strW) return 0;
    }
    MultiByteToWideChar(CP_ACP, 0, str, len, strW, lenW);
    CharUpperBuffW(strW, lenW);
    len = WideCharToMultiByte(CP_ACP, 0, strW, lenW, str, len, NULL, NULL);
    if (strW != buffer) HeapFree(GetProcessHeap(), 0, strW);
    return len;
}

DWORD WINAPI CharUpperBuffW(WCHAR *str, DWORD len)
{
    if (!str) return 0; /* YES */
    return LCMapStringW(LOCALE_USER_DEFAULT, LCMAP_UPPERCASE, str, len, str, len);
}

LPWSTR WINAPI CharUpperW(WCHAR *str)
{
    if (!IS_INTRESOURCE(str))
    {
        CharUpperBuffW(str, lstrlenW(str));
        return str;
    }
    else
    {
        WCHAR ch = LOWORD(str);
        CharUpperBuffW(&ch, 1);
        return (LPWSTR)(UINT_PTR)ch;
    }
}

INT WINAPI DECLSPEC_HOTPATCH LoadStringW(HINSTANCE instance, UINT resource_id, LPWSTR buffer, INT buflen)
{
    int string_num, i;
    HGLOBAL hmem;
    HRSRC hrsrc;
    WCHAR *p;

    TRACE("instance = %p, id = %04x, buffer = %p, length = %d\n", instance, resource_id, buffer, buflen);

    if (!buffer)
        return 0;

    /* Use loword (incremented by 1) as resourceid */
    hrsrc = FindResourceW(instance, MAKEINTRESOURCEW((LOWORD(resource_id) >> 4) + 1), (LPWSTR)RT_STRING);
    if (!hrsrc) return 0;
    hmem = LoadResource(instance, hrsrc);
    if (!hmem) return 0;

    p = LockResource(hmem);
    string_num = resource_id & 0x000f;
    for (i = 0; i < string_num; i++)
        p += *p + 1;

    TRACE("strlen = %d\n", (int)*p );

    /*if buflen == 0, then return a read-only pointer to the resource itself in buffer
    it is assumed that buffer is actually a (LPWSTR *) */
    if (buflen == 0)
    {
        *((LPWSTR *)buffer) = p + 1;
        return *p;
    }

    i = min(buflen - 1, *p);
    if (i > 0)
    {
        memcpy(buffer, p + 1, i * sizeof (WCHAR));
        buffer[i] = 0;
    }
    else
    {
        if (buflen > 1)
        {
            buffer[0] = 0;
            return 0;
        }
    }

    TRACE("returning %s\n", debugstr_w(buffer));
    return i;
}

INT WINAPI DECLSPEC_HOTPATCH LoadStringA(HINSTANCE instance, UINT resource_id, LPSTR buffer, INT buflen)
{
    DWORD retval = 0;
    HGLOBAL hmem;
    HRSRC hrsrc;

    TRACE("instance = %p, id = %04x, buffer = %p, length = %d\n", instance, resource_id, buffer, buflen);

    if (!buflen) return -1;

    /* Use loword (incremented by 1) as resourceid */
    if ((hrsrc = FindResourceW(instance, MAKEINTRESOURCEW((LOWORD(resource_id) >> 4) + 1), (LPWSTR)RT_STRING )) &&
            (hmem = LoadResource(instance, hrsrc)))
    {
        const WCHAR *p = LockResource(hmem);
        unsigned int id = resource_id & 0x000f;

        while (id--) p += *p + 1;

        if (buflen != 1)
            RtlUnicodeToMultiByteN(buffer, buflen - 1, &retval, p + 1, *p * sizeof(WCHAR));
    }
    buffer[retval] = 0;
    TRACE("returning %s\n", debugstr_a(buffer));
    return retval;
}

int WINAPI StrCmpLogicalW(const WCHAR *str, const WCHAR *comp)
{
    TRACE("%s, %s\n", wine_dbgstr_w(str), wine_dbgstr_w(comp));

    if (!str || !comp)
        return 0;

    while (*str)
    {
        if (!*comp)
            return 1;
        else if (isdigitW(*str))
        {
            int str_value, comp_value;

            if (!isdigitW(*comp))
                return -1;

            /* Compare the numbers */
            StrToIntExW(str, 0, &str_value);
            StrToIntExW(comp, 0, &comp_value);

            if (str_value < comp_value)
                return -1;
            else if (str_value > comp_value)
                return 1;

            /* Skip */
            while (isdigitW(*str))
                str++;
            while (isdigitW(*comp))
                comp++;
        }
        else if (isdigitW(*comp))
            return 1;
        else
        {
            int diff = ChrCmpIW(*str, *comp);
            if (diff > 0)
                return 1;
            else if (diff < 0)
                return -1;

            str++;
            comp++;
        }
    }

    if (*comp)
      return -1;

    return 0;
}

BOOL WINAPI StrIsIntlEqualA(BOOL case_sensitive, const char *str, const char *cmp, int len)
{
    DWORD flags;

    TRACE("%d, %s, %s, %d\n", case_sensitive, wine_dbgstr_a(str), wine_dbgstr_a(cmp), len);

    /* FIXME: This flag is undocumented and unknown by our CompareString.
     *        We need a define for it.
     */
    flags = 0x10000000;
    if (!case_sensitive)
        flags |= NORM_IGNORECASE;

    return (CompareStringA(GetThreadLocale(), flags, str, len, cmp, len) == CSTR_EQUAL);
}

BOOL WINAPI StrIsIntlEqualW(BOOL case_sensitive, const WCHAR *str, const WCHAR *cmp, int len)
{
    DWORD flags;

    TRACE("%d, %s, %s, %d\n", case_sensitive, debugstr_w(str), debugstr_w(cmp), len);

    /* FIXME: This flag is undocumented and unknown by our CompareString.
     *        We need a define for it.
     */
    flags = 0x10000000;
    if (!case_sensitive)
        flags |= NORM_IGNORECASE;

    return (CompareStringW(GetThreadLocale(), flags, str, len, cmp, len) == CSTR_EQUAL);
}

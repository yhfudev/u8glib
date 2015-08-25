/**
 * @file    fontutf8.c
 * @brief   font api for u8g lib
 * @author  Yunhui Fu (yhfudev@gmail.com)
 * @version 1.0
 * @date    2015-02-19
 * @copyright GPL/BSD
 */

#include <string.h>

#include "u8g.h"

#if ! defined(__AVR__)
//#define pgm_read_word_near(a) *((uint16_t *)(a))
#define pgm_read_word_near(a) (*(a))
#define pgm_read_byte_near(a) *((uint8_t *)(a))
#define memcpy_P memcpy
#endif

#define font_t void
typedef void (* fontgroup_cb_draw_t)(void *userdata, font_t *fnt_current, const char *msg);
//extern int fontgroup_init (font_group_t * root, u8g_fontinfo_t * fntinfo, int number);
//extern int fontgroup_drawstring (font_group_t * group, font_t *fnt_default, const char *utf8_msg, void * userdata, fontgroup_cb_draw_t cb_draw);
//extern u8g_fontinfo_t * fontgroup_first (font_group_t * root);

#if defined(ARDUINO)
// there's overflow of the wchar_t due to the 2-byte size in Arduino
// sizeof(wchar_t)=2; sizeof(size_t)=2; sizeof(uint32_t)=4;
// sizeof(int)=2; sizeof(long)=4; sizeof(unsigned)=2;
#define wchar_t uint32_t
#else
// x86_64
// sizeof(wchar_t)=4; sizeof(size_t)=8; sizeof(uint32_t)=4;
// sizeof(int)=4; sizeof(long)=8; sizeof(unsigned)=4;
#endif

#define FALSE 0
#define TRUE  1

#if DEBUG
#include <stdio.h>
#include <stdlib.h>
#define assert(a) if (!(a)) {printf("Assert: " # a); exit(1);}
#define TRACE(fmt, ...) fprintf (stdout, "[%s()] " fmt " {ln:%d, fn:" __FILE__ "}\n", __func__, ##__VA_ARGS__, __LINE__)
#else
#define assert(a)
#define TRACE(...)
#endif


#ifdef __WIN32__                // or whatever
#define PRIiSZ "ld"
#define PRIuSZ "Iu"
#else
#define PRIiSZ "zd"
#define PRIuSZ "zu"
#endif
#define PRIiOFF "lld"
#define PRIuOFF "llu"

#define DBGMSG(a,b, ...) TRACE( #__VA_ARGS__ )

typedef int (* pf_bsearch_cb_comp_t)(void *userdata, size_t idx, void * data_pin); /*"data_list[idx] - *data_pin"*/
/**
 * @brief 折半方式查找记录
 *
 * @param userdata : 用户数据指针
 * @param num_data : 数据个数
 * @param cb_comp : 比较两个数据的回调函数
 * @param data_pinpoint : 所要查找的 匹配数据指针
 * @param ret_idx : 查找到的位置;如果没有找到，则返回如添加该记录时其所在的位置。
 *
 * @return 找到则返回0，否则返回<0
 *
 * 折半方式查找记录, psl->marr 中指向的数据已经以先小后大方式排好序
 */
/**
 * @brief Using binary search to find the position by data_pin
 *
 * @param userdata : User's data
 * @param num_data : the item number of the sorted data
 * @param cb_comp : the callback function to compare the user's data and pin
 * @param data_pin : The reference data to be found
 * @param ret_idx : the position of the required data; If failed, then it is the failed position, which is the insert position if possible.
 *
 * @return 0 on found, <0 on failed(fail position is saved by ret_idx)
 *
 * Using binary search to find the position by data_pin. The user's data should be sorted.
 */
int
pf_bsearch_r (void *userdata, size_t num_data, pf_bsearch_cb_comp_t cb_comp, void *data_pinpoint, size_t *ret_idx)
{
    int retcomp;
    uint8_t flg_found;
    size_t ileft;
    size_t iright;
    size_t i;

    assert (NULL != ret_idx);
    /* 查找合适的位置 */
    if (num_data < 1) {
        *ret_idx = 0;
        DBGMSG (PFDBG_CATLOG_PF, PFDBG_LEVEL_ERROR, "num_data(%"PRIuSZ") < 1", num_data);
        return -1;
    }

    /* 折半查找 */
    /* 为了不出现负数，以免缩小索引的所表示的数据范围
     * (负数表明减少一位二进制位的使用)，
     * 内部 ileft 和 iright使用从1开始的下标，
     *   即1表示C语言中的0, 2表示语言中的1，以此类推。
     * 对外还是使用以 0 为开始的下标
     */
    i = 0;
    ileft = 1;
    iright = num_data;
    flg_found = 0;
    for (; ileft <= iright;) {
        i = (ileft + iright) / 2 - 1;
        /* cb_comp should return the *userdata[i] - *data_pinpoint */
        retcomp = cb_comp (userdata, i, data_pinpoint);
        if (retcomp > 0) {
            iright = i;
        } else if (retcomp < 0) {
            ileft = i + 2;
        } else {
            /* found ! */
            flg_found = 1;
            break;
        }
    }

    if (flg_found) {
        *ret_idx = i;
        return 0;
    }
    if (iright <= i) {
        *ret_idx = i;
    } else if (ileft >= i + 2) {
        *ret_idx = i + 1;
    }
    DBGMSG (PFDBG_CATLOG_PF, PFDBG_LEVEL_DEBUG, "not found! num_data=%"PRIuSZ"; ileft=%"PRIuSZ", iright=%"PRIuSZ", i=%"PRIuSZ"", num_data, ileft, iright, i);
    return -1;
}

static wchar_t
get_val_utf82uni (uint8_t *pstart)
{
    size_t cntleft;
    wchar_t retval = 0;

    if (0 == (0x80 & *pstart)) {
        return *pstart;
    }

    if (((*pstart & 0xE0) ^ 0xC0) == 0) {
        cntleft = 1;
        retval = *pstart & ~0xE0;
    } else if (((*pstart & 0xF0) ^ 0xE0) == 0) {
        cntleft = 2;
        retval = *pstart & ~0xF0;
    } else if (((*pstart & 0xF8) ^ 0xF0) == 0) {
        cntleft = 3;
        retval = *pstart & ~0xF8;
    } else if (((*pstart & 0xFC) ^ 0xF8) == 0) {
        cntleft = 4;
        retval = *pstart & ~0xFC;
    } else if (((*pstart & 0xFE) ^ 0xFC) == 0) {
        cntleft = 5;
        retval = *pstart & ~0xFE;
    } else {
        /* encoding error */
        cntleft = 0;
        retval = 0;
    }
    pstart ++;
    for (; cntleft > 0; cntleft --) {
        retval <<= 6;
        retval |= *pstart & 0x3F;
        pstart ++;
    }
    return retval;
}

/**
 * @brief 转换 UTF-8 编码的一个字符为本地的 Unicode 字符(wchar_t)
 *
 * @param pstart : 存储 UTF-8 字符的指针
 * @param pval : 需要返回的 Unicode 字符存放地址指针
 *
 * @return 成功返回下个 UTF-8 字符的位置
 *
 * 转换 UTF-8 编码的一个字符为本地的 Unicode 字符(wchar_t)
 */
static uint8_t *
get_utf8_value (uint8_t *pstart, wchar_t *pval)
{
    uint32_t val = 0;
    uint8_t *p = pstart;
    /*size_t maxlen = strlen (pstart);*/

    assert (NULL != pstart);

    if (0 == (0x80 & *p)) {
        val = (size_t)*p;
        p ++;
    } else if (0xC0 == (0xE0 & *p)) {
        val = *p & 0x1F;
        val <<= 6;
        p ++;
        val |= (*p & 0x3F);
        p ++;
        assert ((wchar_t)val == get_val_utf82uni (pstart));
    } else if (0xE0 == (0xF0 & *p)) {
        val = *p & 0x0F;
        val <<= 6; p ++;
        val |= (*p & 0x3F);
        val <<= 6; p ++;
        val |= (*p & 0x3F);
        p ++;
        assert ((wchar_t)val == get_val_utf82uni (pstart));
    } else if (0xF0 == (0xF8 & *p)) {
        val = *p & 0x07;
        val <<= 6; p ++;
        val |= (*p & 0x3F);
        val <<= 6; p ++;
        val |= (*p & 0x3F);
        val <<= 6; p ++;
        val |= (*p & 0x3F);
        p ++;
        assert ((wchar_t)val == get_val_utf82uni (pstart));
    } else if (0xF8 == (0xFC & *p)) {
        val = *p & 0x03;
        val <<= 6; p ++;
        val |= (*p & 0x3F);
        val <<= 6; p ++;
        val |= (*p & 0x3F);
        val <<= 6; p ++;
        val |= (*p & 0x3F);
        val <<= 6; p ++;
        val |= (*p & 0x3F);
        p ++;
        assert ((wchar_t)val == get_val_utf82uni (pstart));
    } else if (0xFC == (0xFE & *p)) {
        val = *p & 0x01;
        val <<= 6; p ++;
        val |= (*p & 0x3F);
        val <<= 6; p ++;
        val |= (*p & 0x3F);
        val <<= 6; p ++;
        val |= (*p & 0x3F);
        val <<= 6; p ++;
        val |= (*p & 0x3F);
        val <<= 6; p ++;
        val |= (*p & 0x3F);
        p ++;
        assert ((wchar_t)val == get_val_utf82uni (pstart));
    } else if (0x80 == (0xC0 & *p)) {
        /* error? */
        for (; 0x80 == (0xC0 & *p); p ++);
    } else {
        /* error */
        for (; ((0xFE & *p) > 0xFC); p ++);
    }
    /*if (val == 0) {
        p = NULL;*/
/*
    } else if (pstart + maxlen < p) {
        p = pstart;
        if (pval) *pval = 0;
    }
*/

    if (pval) *pval = val;

    return p;
}

/* return v1 - v2 */
static int
fontinfo_compare (u8g_fontinfo_t * v1, u8g_fontinfo_t * v2)
{
    assert (NULL != v1);
    assert (NULL != v2);
    if (v1->page < v2->page) {
        return -1;
    } else if (v1->page > v2->page) {
        return 1;
    }
    if (v1->end < v2->begin) {
        return -1;
    } else if (v1->begin > v2->end) {
        return 1;
    }
    return 0;
}

/*"data_list[idx] - *data_pin"*/
static int
pf_bsearch_cb_comp_fntifo_pgm (void *userdata, size_t idx, void * data_pin)
{
    u8g_fontinfo_t * fntinfo = (u8g_fontinfo_t *) userdata;
    u8g_fontinfo_t localval;
    memcpy_P (&localval, fntinfo + idx, sizeof (localval));
    return fontinfo_compare (&localval, data_pin);
}

typedef struct _font_group_t {
    u8g_fontinfo_t * m_fntifo;
    int m_fntinfo_num;
} font_group_t;

static int
fontgroup_init (font_group_t * root, u8g_fontinfo_t * fntinfo, int number)
{
    root->m_fntifo = fntinfo;
    root->m_fntinfo_num = number;

    return 0;
}

static const font_t *
fontgroup_find (font_group_t * root, wchar_t val)
{
    u8g_fontinfo_t vcmp = {val / 128, val % 128 + 128, val % 128 + 128, 0, 0};
    size_t idx = 0;

    if (val < 128) {
        return U8G_DEFAULT_FONT;
    }

    if (pf_bsearch_r ((void *)root->m_fntifo, root->m_fntinfo_num, pf_bsearch_cb_comp_fntifo_pgm, (void *)&vcmp, &idx) < 0) {
        return NULL;
    }
    memcpy_P (&vcmp, root->m_fntifo + idx, sizeof (vcmp));
    return vcmp.fntdata;
}

static int
fontgroup_drawstring (font_group_t * group, font_t *fnt_default, const char *utf8_msg, void * userdata, fontgroup_cb_draw_t cb_draw)
{
    int len;
    uint8_t *pend = NULL;
    uint8_t *p;
    wchar_t val;
    uint8_t buf[2] = {0, 0};
    font_t * fntpqm = NULL;

    len = strlen(utf8_msg);
    pend = (uint8_t *)utf8_msg + len;
    for (p = (uint8_t *)utf8_msg; p < pend; ) {
        val = 0;
        p = get_utf8_value(p, &val);
        if (NULL == p) {
            TRACE("No more char, break ...");
            break;
        }
        //TRACE("got char=%d", (int)val);
        buf[0] = (uint8_t)(val & 0x7F);
        fntpqm = (font_t *)fontgroup_find (group, val);
        if (NULL == fntpqm) {
            //continue;
            //buf[0] = '?';
            fntpqm = fnt_default;
            TRACE("Unknown char, use default font");
        }
        if (fnt_default != fntpqm) {
            buf[0] |= 0x80; // use upper page to avoid 0x00 error in C. you may want to generate the font data
        }
        //TRACE("set font: %p; (default=%p)", fntpqm, U8G_DEFAULT_FONT);

        cb_draw(userdata, fntpqm, (char *) buf);
    }
}


char flag_fontgroup_inited = 0;
static font_group_t g_fontgroup_root = {NULL, 0};

/**
 * @brief check if font is loaded
 */
char
u8g_Utf8FontIsInited(void)
{
    return flag_fontgroup_inited;
}

int
u8g_SetUtf8Fonts (const u8g_fontinfo_t * fntinfo, int number)
{
    flag_fontgroup_inited = 1;
    return fontgroup_init (&g_fontgroup_root, fntinfo, number);
}

struct _u8g_drawu8_data_t {
    u8g_t *pu8g;
    int x;
    int y;
    void * fnt_prev;
};

static void
fontgroup_cb_draw_u8g (void *userdata, font_t *fnt_current, const char *msg)
{
    struct _u8g_drawu8_data_t * pdata = userdata;

    assert (NULL != userdata);
    if (pdata->fnt_prev != fnt_current) {
        u8g_SetFont (pdata->pu8g, fnt_current);
        u8g_SetFontPosBottom (pdata->pu8g);
        pdata->fnt_prev = fnt_current;
    }
    u8g_DrawStr(pdata->pu8g, pdata->x, pdata->y, (char *) msg);
    pdata->x += u8g_GetStrWidth(pdata->pu8g, (char *)msg);
    //TRACE("next pos= %d", pdata->x);
}

/**
 * @brief draw a UTF-8 string
 */
void
u8g_DrawUtf8Str (u8g_t *pu8g, unsigned int x, unsigned int y, const char *utf8_msg)
{
    struct _u8g_drawu8_data_t data;
    font_group_t * group = &g_fontgroup_root;
    font_t *fnt_default = U8G_DEFAULT_FONT;

    if (! u8g_Utf8FontIsInited()) {
        u8g_DrawStr (pu8g, x, y, "Err: utf8 font not initialized.");
        return;
    }
    data.pu8g = pu8g;
    data.x = x;
    data.y = y;
    data.fnt_prev = NULL;
    fontgroup_drawstring (group, fnt_default, utf8_msg, (void *)&data, fontgroup_cb_draw_u8g);
}

static void
fontgroup_cb_drawp_u8g (void *userdata, font_t *fnt_current, const char *msg)
{
    struct _u8g_drawu8_data_t * pdata = userdata;

    assert (NULL != userdata);
    if (pdata->fnt_prev != fnt_current) {
        u8g_SetFont (pdata->pu8g, fnt_current);
        u8g_SetFontPosBottom (pdata->pu8g);
        pdata->fnt_prev = fnt_current;
    }
    u8g_DrawStrP(pdata->pu8g, pdata->x, pdata->y, (char *) msg);
    pdata->x += u8g_GetStrWidthP(pdata->pu8g, (char *)msg);
    //TRACE("next pos= %d", pdata->x);
}

void
u8g_DrawUtf8StrP (u8g_t *pu8g, unsigned int x, unsigned int y, const char *utf8_msg)
{
    struct _u8g_drawu8_data_t data;
    font_group_t * group = &g_fontgroup_root;
    font_t *fnt_default = U8G_DEFAULT_FONT;

    if (! u8g_Utf8FontIsInited()) {
        u8g_DrawStr (pu8g, x, y, "Err: utf8 font not initialized.");
        return;
    }
    data.pu8g = pu8g;
    data.x = x;
    data.y = y;
    data.fnt_prev = NULL;
    fontgroup_drawstring (group, fnt_default, utf8_msg, (void *)&data, fontgroup_cb_drawp_u8g);
}

static void
fontgroup_cb_draw_u8gstrlen (void *userdata, font_t *fnt_current, const char *msg)
{
    struct _u8g_drawu8_data_t * pdata = userdata;

    assert (NULL != userdata);
    if (pdata->fnt_prev != fnt_current) {
        u8g_SetFont (pdata->pu8g, fnt_current);
        u8g_SetFontPosBottom (pdata->pu8g);
        pdata->fnt_prev = fnt_current;
    }
    pdata->x += u8g_GetStrPixelWidth(pdata->pu8g, (char *)msg);
}

int
u8g_GetUtf8StrPixelWidth(u8g_t *pu8g, char *utf8_msg)
{
    struct _u8g_drawu8_data_t data;
    font_group_t * group = &g_fontgroup_root;
    font_t *fnt_default = U8G_DEFAULT_FONT;

    if (! u8g_Utf8FontIsInited()) {
        TRACE("Err: utf8 font not initialized.");
        return -1;
    }

    memset (&data, 0, sizeof (data));
    data.pu8g = pu8g;
    fontgroup_drawstring (group, fnt_default, utf8_msg, (void *)&data, fontgroup_cb_draw_u8gstrlen);
    return data.x;
}

static void
fontgroup_cb_drawp_u8gstrlen (void *userdata, font_t *fnt_current, const char *msg)
{
    struct _u8g_drawu8_data_t * pdata = userdata;

    assert (NULL != userdata);
    if (pdata->fnt_prev != fnt_current) {
        u8g_SetFont (pdata->pu8g, fnt_current);
        u8g_SetFontPosBottom (pdata->pu8g);
        pdata->fnt_prev = fnt_current;
    }
    pdata->x += u8g_GetStrPixelWidthP(pdata->pu8g, (char *)msg);
}

int
u8g_GetUtf8StrPixelWidthP(u8g_t *pu8g, char *utf8_msg)
{
    struct _u8g_drawu8_data_t data;
    font_group_t * group = &g_fontgroup_root;
    font_t *fnt_default = U8G_DEFAULT_FONT;

    if (! u8g_Utf8FontIsInited()) {
        TRACE("Err: utf8 font not initialized.");
        return -1;
    }
    memset (&data, 0, sizeof (data));
    data.pu8g = pu8g;
    fontgroup_drawstring (group, fnt_default, utf8_msg, (void *)&data, fontgroup_cb_drawp_u8gstrlen);
    return data.x;
}

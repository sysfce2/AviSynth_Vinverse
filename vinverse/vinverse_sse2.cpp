#include "vinverse.h"
#include "VCL2/vectorclass.h"

void vertical_blur3_sse2(uint8_t* dstp, const uint8_t* srcp, int dst_pitch, int src_pitch, int width, int height)
{
    int mod16_width = (width + 15) / 16 * 16;

    auto zero = _mm_setzero_si128();
    auto two = _mm_set1_epi16(2);

    for (int y = 0; y < height; ++y)
    {
        const uint8_t* srcpp = y == 0 ? srcp + src_pitch : srcp - src_pitch;
        const uint8_t* srcpn = y == height - 1 ? srcp - src_pitch : srcp + src_pitch;

        for (int x = 0; x < mod16_width; x += 16)
        {
            auto p = _mm_load_si128(reinterpret_cast<const __m128i*>(srcpp + x));
            auto c = _mm_load_si128(reinterpret_cast<const __m128i*>(srcp + x));
            auto n = _mm_load_si128(reinterpret_cast<const __m128i*>(srcpn + x));

            auto p_lo = _mm_unpacklo_epi8(p, zero);
            auto p_hi = _mm_unpackhi_epi8(p, zero);
            auto c_lo = _mm_unpacklo_epi8(c, zero);
            auto c_hi = _mm_unpackhi_epi8(c, zero);
            auto n_lo = _mm_unpacklo_epi8(n, zero);
            auto n_hi = _mm_unpackhi_epi8(n, zero);

            auto acc_lo = _mm_add_epi16(c_lo, p_lo);
            auto acc_hi = _mm_add_epi16(c_hi, p_hi);

            acc_lo = _mm_add_epi16(acc_lo, c_lo);
            acc_hi = _mm_add_epi16(acc_hi, c_hi);

            acc_lo = _mm_add_epi16(acc_lo, n_lo);
            acc_hi = _mm_add_epi16(acc_hi, n_hi);

            acc_lo = _mm_add_epi16(acc_lo, two);
            acc_hi = _mm_add_epi16(acc_hi, two);

            acc_lo = _mm_srli_epi16(acc_lo, 2);
            acc_hi = _mm_srli_epi16(acc_hi, 2);

            auto dst = _mm_packus_epi16(acc_lo, acc_hi);

            _mm_store_si128(reinterpret_cast<__m128i*>(dstp + x), dst);
        }

        srcp += src_pitch;
        dstp += dst_pitch;
    }
}

void vertical_blur5_sse2(uint8_t* dstp, const uint8_t* srcp, int dst_pitch, int src_pitch, int width, int height)
{
    int mod16_width = (width + 15) / 16 * 16;

    auto zero = _mm_setzero_si128();
    auto six = _mm_set1_epi16(6);
    auto eight = _mm_set1_epi16(8);

    for (int y = 0; y < height; ++y)
    {
        const uint8_t* srcppp = y < 2 ? srcp + src_pitch * 2 : srcp - src_pitch * 2;
        const uint8_t* srcpp = y == 0 ? srcp + src_pitch : srcp - src_pitch;
        const uint8_t* srcpn = y == height - 1 ? srcp - src_pitch : srcp + src_pitch;
        const uint8_t* srcpnn = y > height - 3 ? srcp - src_pitch * 2 : srcp + src_pitch * 2;

        for (int x = 0; x < mod16_width; x += 16)
        {
            auto p2 = _mm_load_si128(reinterpret_cast<const __m128i*>(srcppp + x));
            auto p1 = _mm_load_si128(reinterpret_cast<const __m128i*>(srcpp + x));
            auto c = _mm_load_si128(reinterpret_cast<const __m128i*>(srcp + x));
            auto n1 = _mm_load_si128(reinterpret_cast<const __m128i*>(srcpn + x));
            auto n2 = _mm_load_si128(reinterpret_cast<const __m128i*>(srcpnn + x));

            auto p2_lo = _mm_unpacklo_epi8(p2, zero);
            auto p2_hi = _mm_unpackhi_epi8(p2, zero);
            auto p1_lo = _mm_unpacklo_epi8(p1, zero);
            auto p1_hi = _mm_unpackhi_epi8(p1, zero);
            auto c_lo = _mm_unpacklo_epi8(c, zero);
            auto c_hi = _mm_unpackhi_epi8(c, zero);
            auto n1_lo = _mm_unpacklo_epi8(n1, zero);
            auto n1_hi = _mm_unpackhi_epi8(n1, zero);
            auto n2_lo = _mm_unpacklo_epi8(n2, zero);
            auto n2_hi = _mm_unpackhi_epi8(n2, zero);

            auto acc_lo = _mm_mullo_epi16(c_lo, six);
            auto acc_hi = _mm_mullo_epi16(c_hi, six);

            auto t_lo = _mm_add_epi16(p1_lo, n1_lo);
            auto t_hi = _mm_add_epi16(p1_hi, n1_hi);

            acc_lo = _mm_add_epi16(acc_lo, n2_lo);
            acc_hi = _mm_add_epi16(acc_hi, n2_hi);

            t_lo = _mm_slli_epi16(t_lo, 2);
            t_hi = _mm_slli_epi16(t_hi, 2);

            t_lo = _mm_add_epi16(t_lo, p2_lo);
            t_hi = _mm_add_epi16(t_hi, p2_hi);

            acc_lo = _mm_add_epi16(acc_lo, eight);
            acc_hi = _mm_add_epi16(acc_hi, eight);

            acc_lo = _mm_add_epi16(acc_lo, t_lo);
            acc_hi = _mm_add_epi16(acc_hi, t_hi);

            acc_lo = _mm_srli_epi16(acc_lo, 4);
            acc_hi = _mm_srli_epi16(acc_hi, 4);

            auto dst = _mm_packus_epi16(acc_lo, acc_hi);
            _mm_store_si128(reinterpret_cast<__m128i*>(dstp + x), dst);
        }

        srcp += src_pitch;
        dstp += dst_pitch;
    }
}

static void mt_makediff_sse2(uint8_t* dstp, const uint8_t* c1p, const uint8_t* c2p, int dst_pitch, int c1_pitch, int c2_pitch, int width, int height)
{
    int mod16_width = (width + 15) / 16 * 16;

    __m128i v128 = _mm_set1_epi32(0x80808080);
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < mod16_width; x += 16)
        {
            __m128i c1 = _mm_load_si128(reinterpret_cast<const __m128i*>(c1p + x));
            __m128i c2 = _mm_load_si128(reinterpret_cast<const __m128i*>(c2p + x));

            c1 = _mm_sub_epi8(c1, v128);
            c2 = _mm_sub_epi8(c2, v128);

            __m128i diff = _mm_subs_epi8(c1, c2);
            diff = _mm_add_epi8(diff, v128);

            _mm_store_si128(reinterpret_cast<__m128i*>(dstp + x), diff);
        }
        dstp += dst_pitch;
        c1p += c1_pitch;
        c2p += c2_pitch;
    }
}

//mask ? a : b
static AVS_FORCEINLINE __m128i blend_si128(__m128i const& mask, __m128i const& desired, __m128i const& otherwise)
{
    //return _mm_blendv_ps(otherwise, desired, mask);
    auto andop = _mm_and_si128(mask, desired);
    auto andnop = _mm_andnot_si128(mask, otherwise);
    return _mm_or_si128(andop, andnop);
}

static AVS_FORCEINLINE __m128i abs_epi16(const __m128i& src, const __m128i& zero)
{
    __m128i is_negative = _mm_cmplt_epi16(src, zero);
    __m128i reversed = _mm_subs_epi16(zero, src);
    return blend_si128(is_negative, reversed, src);
}

void vertical_sbr_sse2(uint8_t* dstp, uint8_t* tempp, const uint8_t* srcp, int dst_pitch, int temp_pitch, int src_pitch, int width, int height)
{
    vertical_blur3_sse2(tempp, srcp, temp_pitch, src_pitch, width, height); //temp = rg11
    mt_makediff_sse2(dstp, srcp, tempp, dst_pitch, src_pitch, temp_pitch, width, height); //dst = rg11D
    vertical_blur3_sse2(tempp, dstp, temp_pitch, dst_pitch, width, height); //temp = rg11D.vblur()

    int mod8_width = (width + 7) / 8 * 8;

    __m128i zero = _mm_setzero_si128();
    __m128i v128 = _mm_set1_epi16(128);

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < mod8_width; x += 8)
        {
            __m128i dst = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(dstp + x));
            __m128i temp = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(tempp + x));
            __m128i src = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcp + x));

            dst = _mm_unpacklo_epi8(dst, zero);
            temp = _mm_unpacklo_epi8(temp, zero);
            src = _mm_unpacklo_epi8(src, zero);

            __m128i t = _mm_subs_epi16(dst, temp);
            __m128i t2 = _mm_subs_epi16(dst, v128);

            __m128i nochange_mask = _mm_cmplt_epi16(_mm_mullo_epi16(t, t2), zero);

            __m128i t_mask = _mm_cmplt_epi16(abs_epi16(t, zero), abs_epi16(t2, zero));
            __m128i desired = _mm_subs_epi16(src, t);
            __m128i otherwise = _mm_add_epi16(_mm_subs_epi16(src, dst), v128);
            __m128i result = blend_si128(nochange_mask, src, blend_si128(t_mask, desired, otherwise));

            result = _mm_packus_epi16(result, zero);

            _mm_storel_epi64(reinterpret_cast<__m128i*>(dstp + x), result);
        }
        dstp += dst_pitch;
        srcp += src_pitch;
        tempp += temp_pitch;
    }
}

AVS_FORCEINLINE __m128 abs_ps(const __m128& x)
{
    static const __m128 sign_mask = _mm_set_ps1(-0.0f); // -0.f = 1 << 31
    return _mm_andnot_ps(sign_mask, x);
}

//mask ? a : b
static AVS_FORCEINLINE __m128 blend_ps(__m128 const& mask, __m128 const& desired, __m128 const& otherwise)
{
    //return _mm_blendv_ps(otherwise, desired, mask);
    auto andop = _mm_and_ps(mask, desired);
    auto andnop = _mm_andnot_ps(mask, otherwise);
    return _mm_or_ps(andop, andnop);
}

void finalize_plane_sse2(uint8_t* dstp, const uint8_t* srcp, const uint8_t* pb3, const uint8_t* pb6, float sstr, float scl, int src_pitch, int dst_pitch, int pb_pitch, int width, int height, int amnt)
{
    int mod8_width = (width + 7) / 8 * 8;

    auto zero = _mm_setzero_si128();
    auto sstr_vector = _mm_set_ps1(sstr);
    auto scl_vector = _mm_set_ps1(scl);
    auto amnt_vector = _mm_set1_epi16(amnt);

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < mod8_width; x += 8)
        {
            __m128i b3 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(pb3 + x));
            __m128i b6 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(pb6 + x));
            __m128i src = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcp + x));

            b3 = _mm_unpacklo_epi8(b3, zero);
            b6 = _mm_unpacklo_epi8(b6, zero);
            src = _mm_unpacklo_epi8(src, zero);

            auto d1i = _mm_subs_epi16(src, b3);
            auto d1i_sign = _mm_cmplt_epi16(d1i, zero);

            auto d2i = _mm_subs_epi16(b3, b6);
            auto d2i_sign = _mm_cmplt_epi16(d2i, zero);

            auto d1_lo = _mm_cvtepi32_ps(_mm_unpacklo_epi16(d1i, d1i_sign));
            auto d1_hi = _mm_cvtepi32_ps(_mm_unpackhi_epi16(d1i, d1i_sign));

            auto d2_lo = _mm_cvtepi32_ps(_mm_unpacklo_epi16(d2i, d2i_sign));
            auto d2_hi = _mm_cvtepi32_ps(_mm_unpackhi_epi16(d2i, d2i_sign));

            auto t_lo = _mm_mul_ps(d2_lo, sstr_vector);
            auto t_hi = _mm_mul_ps(d2_hi, sstr_vector);

            auto da_mask_lo = _mm_cmplt_ps(abs_ps(d1_lo), abs_ps(t_lo));
            auto da_mask_hi = _mm_cmplt_ps(abs_ps(d1_hi), abs_ps(t_hi));

            auto da_lo = blend_ps(da_mask_lo, d1_lo, t_lo);
            auto da_hi = blend_ps(da_mask_hi, d1_hi, t_hi);

            auto desired_lo = _mm_mul_ps(da_lo, scl_vector);
            auto desired_hi = _mm_mul_ps(da_hi, scl_vector);

            auto fin_mask_lo = _mm_cmplt_ps(_mm_mul_ps(d1_lo, t_lo), _mm_castsi128_ps(zero));
            auto fin_mask_hi = _mm_cmplt_ps(_mm_mul_ps(d1_hi, t_hi), _mm_castsi128_ps(zero));

            auto add_lo = _mm_cvttps_epi32(blend_ps(fin_mask_lo, desired_lo, da_lo));
            auto add_hi = _mm_cvttps_epi32(blend_ps(fin_mask_hi, desired_hi, da_hi));

            auto add = _mm_packs_epi32(add_lo, add_hi);
            auto df = _mm_add_epi16(b3, add);

            auto minm = _mm_subs_epi16(src, amnt_vector);
            auto maxf = _mm_adds_epi16(src, amnt_vector);

            df = _mm_max_epi16(df, minm);
            df = _mm_min_epi16(df, maxf);

            auto result = _mm_packus_epi16(df, zero);
            _mm_storel_epi64(reinterpret_cast<__m128i*>(dstp + x), result);
        }
        srcp += src_pitch;
        pb3 += pb_pitch;
        pb6 += pb_pitch;
        dstp += dst_pitch;
    }
}

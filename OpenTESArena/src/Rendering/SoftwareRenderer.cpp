#include <algorithm>
#include <condition_variable>
#include <cmath>
#include <cstring>
#include <deque>
#include <limits>
#include <mutex>
#include <thread>

#include "ArenaRenderUtils.h"
#include "RenderCamera.h"
#include "RenderCommandBuffer.h"
#include "RenderDrawCall.h"
#include "RendererUtils.h"
#include "RenderFrameSettings.h"
#include "RenderInitSettings.h"
#include "RenderTransform.h"
#include "SoftwareRenderer.h"
#include "../Assets/TextureBuilder.h"
#include "../Math/Constants.h"
#include "../Math/MathUtils.h"
#include "../Math/Random.h"
#include "../Utilities/Color.h"
#include "../Utilities/Palette.h"
#include "../World/ChunkUtils.h"

#include "components/debug/Debug.h"

// Loop unroll utils.
namespace
{
	constexpr int TYPICAL_LOOP_UNROLL = 4; // Elements processed per unrolled loop, possibly also for SIMD lanes.
	constexpr int WEAK_LOOP_UNROLL = TYPICAL_LOOP_UNROLL / 2;
	constexpr int AGGRESSIVE_LOOP_UNROLL = TYPICAL_LOOP_UNROLL * 2;
	static_assert(MathUtils::isPowerOf2(TYPICAL_LOOP_UNROLL));
	static_assert(MathUtils::isPowerOf2(WEAK_LOOP_UNROLL));
	static_assert(MathUtils::isPowerOf2(AGGRESSIVE_LOOP_UNROLL));

	int GetUnrollAdjustedLoopCount(int loopCount, int unrollCount)
	{
		return loopCount - (unrollCount - 1);
	}
}

// Optimized math functions.
namespace
{
	template<int N>
	void Double_LerpN(const double *__restrict starts, const double *__restrict ends, const double *__restrict percents,
		double *__restrict outs)
	{
		for (int i = 0; i < N; i++)
		{
			const double start = starts[i];
			const double end = ends[i];
			const double percent = percents[i];
			outs[i] = start + ((end - start) * percent);
		}
	}

	template<int Index0, int Index1, int Index2, int Index3>
	void Double_Shuffle4(const double *__restrict values, double *__restrict outValues)
	{
		outValues[Index0] = values[0];
		outValues[Index1] = values[1];
		outValues[Index2] = values[2];
		outValues[Index3] = values[3];
	}

	template<int N>
	void Double2_DotN(const double *__restrict x0s, const double *__restrict y0s, const double *__restrict x1s, const double *__restrict y1s,
		double *__restrict outs)
	{
		for (int i = 0; i < N; i++)
		{
			outs[i] = (x0s[i] * x1s[i]) + (y0s[i] * y1s[i]);
		}
	}

	template<int N>
	void Double2_CrossN(const double *__restrict x0s, const double *__restrict y0s, const double *__restrict x1s, const double *__restrict y1s,
		double *__restrict outs)
	{
		for (int i = 0; i < N; i++)
		{
			outs[i] = (x0s[i] * y1s[i]) - (y0s[i] * x1s[i]);
		}
	}

	template<int N>
	void Double2_RightPerpN(const double *__restrict xs, const double *__restrict ys, double *__restrict outXs, double *__restrict outYs)
	{
		for (int i = 0; i < N; i++)
		{
			outXs[i] = ys[i];
		}

		for (int i = 0; i < N; i++)
		{
			outYs[i] = -xs[i];
		}
	}

	template<int N>
	void Double4_ZeroN(double *__restrict outXs, double *__restrict outYs, double *__restrict outZs, double *__restrict outWs)
	{
		for (int i = 0; i < N; i++)
		{
			outXs[i] = 0.0;
		}

		for (int i = 0; i < N; i++)
		{
			outYs[i] = 0.0;
		}

		for (int i = 0; i < N; i++)
		{
			outZs[i] = 0.0;
		}

		for (int i = 0; i < N; i++)
		{
			outWs[i] = 0.0;
		}
	}

	void Double4_Load4(const Double4 &v0, const Double4 &v1, const Double4 &v2, const Double4 &v3,
		double *__restrict outXs, double *__restrict outYs, double *__restrict outZs, double *__restrict outWs)
	{
		outXs[0] = v0.x;
		outYs[0] = v0.y;
		outZs[0] = v0.z;
		outWs[0] = v0.w;
		outXs[1] = v1.x;
		outYs[1] = v1.y;
		outZs[1] = v1.z;
		outWs[1] = v1.w;
		outXs[2] = v2.x;
		outYs[2] = v2.y;
		outZs[2] = v2.z;
		outWs[2] = v2.w;
		outXs[3] = v3.x;
		outYs[3] = v3.y;
		outZs[3] = v3.z;
		outWs[3] = v3.w;
	}

	void Double4_Store4(const double *__restrict xs, const double *__restrict ys, const double *__restrict zs, const double *__restrict ws,
		double *__restrict outV0, double *__restrict outV1, double *__restrict outV2, double *__restrict outV3)
	{
		outV0[0] = xs[0];
		outV0[1] = ys[0];
		outV0[2] = zs[0];
		outV0[3] = ws[0];
		outV1[0] = xs[1];
		outV1[1] = ys[1];
		outV1[2] = zs[1];
		outV1[3] = ws[1];
		outV2[0] = xs[2];
		outV2[1] = ys[2];
		outV2[2] = zs[2];
		outV2[3] = ws[2];
		outV3[0] = xs[3];
		outV3[1] = ys[3];
		outV3[2] = zs[3];
		outV3[3] = ws[3];
	}

	template<int N>
	void Double4_AddN(const double *__restrict x0s, const double *__restrict y0s, const double *__restrict z0s, const double *__restrict w0s,
		const double *__restrict x1s, const double *__restrict y1s, const double *__restrict z1s, const double *__restrict w1s,
		double *__restrict outXs, double *__restrict outYs, double *__restrict outZs, double *__restrict outWs)
	{
		for (int i = 0; i < N; i++)
		{
			outXs[i] = x0s[i] + x1s[i];
		}

		for (int i = 0; i < N; i++)
		{
			outYs[i] = y0s[i] + y1s[i];
		}

		for (int i = 0; i < N; i++)
		{
			outZs[i] = z0s[i] + z1s[i];
		}

		for (int i = 0; i < N; i++)
		{
			outWs[i] = w0s[i] + w1s[i];
		}
	}

	template<int N>
	void Double4_NegateN(const double *__restrict xs, const double *__restrict ys, const double *__restrict zs, const double *__restrict ws,
		double *__restrict outXs, double *__restrict outYs, double *__restrict outZs, double *__restrict outWs)
	{
		for (int i = 0; i < N; i++)
		{
			outXs[i] = -xs[i];
		}

		for (int i = 0; i < N; i++)
		{
			outYs[i] = -ys[i];
		}

		for (int i = 0; i < N; i++)
		{
			outZs[i] = -zs[i];
		}

		for (int i = 0; i < N; i++)
		{
			outWs[i] = -ws[i];
		}
	}

	template<int N>
	void Double4_SubtractN(const double *__restrict x0s, const double *__restrict y0s, const double *__restrict z0s, const double *__restrict w0s,
		const double *__restrict x1s, const double *__restrict y1s, const double *__restrict z1s, const double *__restrict w1s,
		double *__restrict outXs, double *__restrict outYs, double *__restrict outZs, double *__restrict outWs)
	{
		for (int i = 0; i < N; i++)
		{
			outXs[i] = x0s[i] - x1s[i];
		}

		for (int i = 0; i < N; i++)
		{
			outYs[i] = y0s[i] - y1s[i];
		}

		for (int i = 0; i < N; i++)
		{
			outZs[i] = z0s[i] - z1s[i];
		}

		for (int i = 0; i < N; i++)
		{
			outWs[i] = w0s[i] - w1s[i];
		}
	}

	template<int N>
	void Double4_MultiplyN(const double *__restrict x0s, const double *__restrict y0s, const double *__restrict z0s, const double *__restrict w0s,
		const double *__restrict x1s, const double *__restrict y1s, const double *__restrict z1s, const double *__restrict w1s,
		double *__restrict outXs, double *__restrict outYs, double *__restrict outZs, double *__restrict outWs)
	{
		for (int i = 0; i < N; i++)
		{
			outXs[i] = x0s[i] * x1s[i];
		}

		for (int i = 0; i < N; i++)
		{
			outYs[i] = y0s[i] * y1s[i];
		}

		for (int i = 0; i < N; i++)
		{
			outZs[i] = z0s[i] * z1s[i];
		}

		for (int i = 0; i < N; i++)
		{
			outWs[i] = w0s[i] * w1s[i];
		}
	}

	template<int N>
	void Double4_DivideN(const double *__restrict x0s, const double *__restrict y0s, const double *__restrict z0s, const double *__restrict w0s,
		const double *__restrict x1s, const double *__restrict y1s, const double *__restrict z1s, const double *__restrict w1s,
		double *__restrict outXs, double *__restrict outYs, double *__restrict outZs, double *__restrict outWs)
	{
		for (int i = 0; i < N; i++)
		{
			outXs[i] = x0s[i] / x1s[i];
		}

		for (int i = 0; i < N; i++)
		{
			outYs[i] = y0s[i] / y1s[i];
		}

		for (int i = 0; i < N; i++)
		{
			outZs[i] = z0s[i] / z1s[i];
		}

		for (int i = 0; i < N; i++)
		{
			outWs[i] = w0s[i] / w1s[i];
		}
	}

	template<int N>
	void Matrix4_ZeroN(double *__restrict outMxxs, double *__restrict outMxys, double *__restrict outMxzs, double *__restrict outMxws,
		double *__restrict outMyxs, double *__restrict outMyys, double *__restrict outMyzs, double *__restrict outMyws,
		double *__restrict outMzxs, double *__restrict outMzys, double *__restrict outMzzs, double *__restrict outMzws,
		double *__restrict outMwxs, double *__restrict outMwys, double *__restrict outMwzs, double *__restrict outMwws)
	{
		for (int i = 0; i < N; i++)
		{
			outMxxs[i] = 0.0;
		}

		for (int i = 0; i < N; i++)
		{
			outMxys[i] = 0.0;
		}

		for (int i = 0; i < N; i++)
		{
			outMxzs[i] = 0.0;
		}

		for (int i = 0; i < N; i++)
		{
			outMxws[i] = 0.0;
		}

		for (int i = 0; i < N; i++)
		{
			outMyxs[i] = 0.0;
		}

		for (int i = 0; i < N; i++)
		{
			outMyys[i] = 0.0;
		}

		for (int i = 0; i < N; i++)
		{
			outMyzs[i] = 0.0;
		}

		for (int i = 0; i < N; i++)
		{
			outMyws[i] = 0.0;
		}

		for (int i = 0; i < N; i++)
		{
			outMzxs[i] = 0.0;
		}

		for (int i = 0; i < N; i++)
		{
			outMzys[i] = 0.0;
		}

		for (int i = 0; i < N; i++)
		{
			outMzzs[i] = 0.0;
		}

		for (int i = 0; i < N; i++)
		{
			outMzws[i] = 0.0;
		}

		for (int i = 0; i < N; i++)
		{
			outMwxs[i] = 0.0;
		}

		for (int i = 0; i < N; i++)
		{
			outMwys[i] = 0.0;
		}

		for (int i = 0; i < N; i++)
		{
			outMwzs[i] = 0.0;
		}

		for (int i = 0; i < N; i++)
		{
			outMwws[i] = 0.0;
		}
	}

	void Matrix4_Load4(const Matrix4d &m0, const Matrix4d &m1, const Matrix4d &m2, const Matrix4d &m3,
		double *__restrict outMxxs, double *__restrict outMxys, double *__restrict outMxzs, double *__restrict outMxws,
		double *__restrict outMyxs, double *__restrict outMyys, double *__restrict outMyzs, double *__restrict outMyws,
		double *__restrict outMzxs, double *__restrict outMzys, double *__restrict outMzzs, double *__restrict outMzws,
		double *__restrict outMwxs, double *__restrict outMwys, double *__restrict outMwzs, double *__restrict outMwws)
	{
		outMxxs[0] = m0.x.x;
		outMxys[0] = m0.x.y;
		outMxzs[0] = m0.x.z;
		outMxws[0] = m0.x.w;
		outMyxs[0] = m0.y.x;
		outMyys[0] = m0.y.y;
		outMyzs[0] = m0.y.z;
		outMyws[0] = m0.y.w;
		outMzxs[0] = m0.z.x;
		outMzys[0] = m0.z.y;
		outMzzs[0] = m0.z.z;
		outMzws[0] = m0.z.w;
		outMwxs[0] = m0.w.x;
		outMwys[0] = m0.w.y;
		outMwzs[0] = m0.w.z;
		outMwws[0] = m0.w.w;
		outMxxs[1] = m1.x.x;
		outMxys[1] = m1.x.y;
		outMxzs[1] = m1.x.z;
		outMxws[1] = m1.x.w;
		outMyxs[1] = m1.y.x;
		outMyys[1] = m1.y.y;
		outMyzs[1] = m1.y.z;
		outMyws[1] = m1.y.w;
		outMzxs[1] = m1.z.x;
		outMzys[1] = m1.z.y;
		outMzzs[1] = m1.z.z;
		outMzws[1] = m1.z.w;
		outMwxs[1] = m1.w.x;
		outMwys[1] = m1.w.y;
		outMwzs[1] = m1.w.z;
		outMwws[1] = m1.w.w;
		outMxxs[2] = m2.x.x;
		outMxys[2] = m2.x.y;
		outMxzs[2] = m2.x.z;
		outMxws[2] = m2.x.w;
		outMyxs[2] = m2.y.x;
		outMyys[2] = m2.y.y;
		outMyzs[2] = m2.y.z;
		outMyws[2] = m2.y.w;
		outMzxs[2] = m2.z.x;
		outMzys[2] = m2.z.y;
		outMzzs[2] = m2.z.z;
		outMzws[2] = m2.z.w;
		outMwxs[2] = m2.w.x;
		outMwys[2] = m2.w.y;
		outMwzs[2] = m2.w.z;
		outMwws[2] = m2.w.w;
		outMxxs[3] = m3.x.x;
		outMxys[3] = m3.x.y;
		outMxzs[3] = m3.x.z;
		outMxws[3] = m3.x.w;
		outMyxs[3] = m3.y.x;
		outMyys[3] = m3.y.y;
		outMyzs[3] = m3.y.z;
		outMyws[3] = m3.y.w;
		outMzxs[3] = m3.z.x;
		outMzys[3] = m3.z.y;
		outMzzs[3] = m3.z.z;
		outMzws[3] = m3.z.w;
		outMwxs[3] = m3.w.x;
		outMwys[3] = m3.w.y;
		outMwzs[3] = m3.w.z;
		outMwws[3] = m3.w.w;
	}

	template<int N>
	void Matrix4_MultiplyVectorN(
		const double *__restrict mxxs, const double *__restrict mxys, const double *__restrict mxzs, const double *__restrict mxws,
		const double *__restrict myxs, const double *__restrict myys, const double *__restrict myzs, const double *__restrict myws,
		const double *__restrict mzxs, const double *__restrict mzys, const double *__restrict mzzs, const double *__restrict mzws,
		const double *__restrict mwxs, const double *__restrict mwys, const double *__restrict mwzs, const double *__restrict mwws,
		const double *__restrict xs, const double *__restrict ys, const double *__restrict zs, const double *__restrict ws,
		double *__restrict outXs, double *__restrict outYs, double *__restrict outZs, double *__restrict outWs)
	{
		for (int i = 0; i < N; i++)
		{
			outXs[i] += (mxxs[i] * xs[i]) + (myxs[i] * ys[i]) + (mzxs[i] * zs[i]) + (mwxs[i] * ws[i]);
		}

		for (int i = 0; i < N; i++)
		{
			outYs[i] += (mxys[i] * xs[i]) + (myys[i] * ys[i]) + (mzys[i] * zs[i]) + (mwys[i] * ws[i]);
		}

		for (int i = 0; i < N; i++)
		{
			outZs[i] += (mxzs[i] * xs[i]) + (myzs[i] * ys[i]) + (mzzs[i] * zs[i]) + (mwzs[i] * ws[i]);
		}

		for (int i = 0; i < N; i++)
		{
			outWs[i] += (mxws[i] * xs[i]) + (myws[i] * ys[i]) + (mzws[i] * zs[i]) + (mwws[i] * ws[i]);
		}
	}

	template<int N>
	void Matrix4_MultiplyVectorIgnoreW_N(
		const double *__restrict mxxs, const double *__restrict mxys, const double *__restrict mxzs,
		const double *__restrict myxs, const double *__restrict myys, const double *__restrict myzs,
		const double *__restrict mzxs, const double *__restrict mzys, const double *__restrict mzzs,
		const double *__restrict mwxs, const double *__restrict mwys, const double *__restrict mwzs,
		const double *__restrict xs, const double *__restrict ys, const double *__restrict zs, const double *__restrict ws,
		double *__restrict outXs, double *__restrict outYs, double *__restrict outZs)
	{
		for (int i = 0; i < N; i++)
		{
			outXs[i] += (mxxs[i] * xs[i]) + (myxs[i] * ys[i]) + (mzxs[i] * zs[i]) + (mwxs[i] * ws[i]);
		}

		for (int i = 0; i < N; i++)
		{
			outYs[i] += (mxys[i] * xs[i]) + (myys[i] * ys[i]) + (mzys[i] * zs[i]) + (mwys[i] * ws[i]);
		}

		for (int i = 0; i < N; i++)
		{
			outZs[i] += (mxzs[i] * xs[i]) + (myzs[i] * ys[i]) + (mzzs[i] * zs[i]) + (mwzs[i] * ws[i]);
		}
	}

	template<int N>
	void Matrix4_MultiplyMatrixN(const double *__restrict m0xxs, const double *__restrict m0xys, const double *__restrict m0xzs, const double *__restrict m0xws,
		const double *__restrict m0yxs, const double *__restrict m0yys, const double *__restrict m0yzs, const double *__restrict m0yws,
		const double *__restrict m0zxs, const double *__restrict m0zys, const double *__restrict m0zzs, const double *__restrict m0zws,
		const double *__restrict m0wxs, const double *__restrict m0wys, const double *__restrict m0wzs, const double *__restrict m0wws,
		const double *__restrict m1xxs, const double *__restrict m1xys, const double *__restrict m1xzs, const double *__restrict m1xws,
		const double *__restrict m1yxs, const double *__restrict m1yys, const double *__restrict m1yzs, const double *__restrict m1yws,
		const double *__restrict m1zxs, const double *__restrict m1zys, const double *__restrict m1zzs, const double *__restrict m1zws,
		const double *__restrict m1wxs, const double *__restrict m1wys, const double *__restrict m1wzs, const double *__restrict m1wws,
		double *__restrict outMxxs, double *__restrict outMxys, double *__restrict outMxzs, double *__restrict outMxws,
		double *__restrict outMyxs, double *__restrict outMyys, double *__restrict outMyzs, double *__restrict outMyws,
		double *__restrict outMzxs, double *__restrict outMzys, double *__restrict outMzzs, double *__restrict outMzws,
		double *__restrict outMwxs, double *__restrict outMwys, double *__restrict outMwzs, double *__restrict outMwws)
	{
		for (int i = 0; i < N; i++)
		{
			outMxxs[i] = (m0xxs[i] * m1xxs[i]) + (m0yxs[i] * m1xys[i]) + (m0zxs[i] * m1xzs[i]) + (m0wxs[i] * m1xws[i]);
		}

		for (int i = 0; i < N; i++)
		{
			outMxys[i] = (m0xys[i] * m1xxs[i]) + (m0yys[i] * m1xys[i]) + (m0zys[i] * m1xzs[i]) + (m0wys[i] * m1xws[i]);
		}

		for (int i = 0; i < N; i++)
		{
			outMxzs[i] = (m0xzs[i] * m1xxs[i]) + (m0yzs[i] * m1xys[i]) + (m0zzs[i] * m1xzs[i]) + (m0wzs[i] * m1xws[i]);
		}

		for (int i = 0; i < N; i++)
		{
			outMxws[i] = (m0xws[i] * m1xxs[i]) + (m0yws[i] * m1xys[i]) + (m0zws[i] * m1xzs[i]) + (m0wws[i] * m1xws[i]);
		}

		for (int i = 0; i < N; i++)
		{
			outMyxs[i] = (m0xxs[i] * m1yxs[i]) + (m0yxs[i] * m1yys[i]) + (m0zxs[i] * m1yzs[i]) + (m0wxs[i] * m1yws[i]);
		}

		for (int i = 0; i < N; i++)
		{
			outMyys[i] = (m0xys[i] * m1yxs[i]) + (m0yys[i] * m1yys[i]) + (m0zys[i] * m1yzs[i]) + (m0wys[i] * m1yws[i]);
		}

		for (int i = 0; i < N; i++)
		{
			outMyzs[i] = (m0xzs[i] * m1yxs[i]) + (m0yzs[i] * m1yys[i]) + (m0zzs[i] * m1yzs[i]) + (m0wzs[i] * m1yws[i]);
		}

		for (int i = 0; i < N; i++)
		{
			outMyws[i] = (m0xws[i] * m1yxs[i]) + (m0yws[i] * m1yys[i]) + (m0zws[i] * m1yzs[i]) + (m0wws[i] * m1yws[i]);
		}

		for (int i = 0; i < N; i++)
		{
			outMzxs[i] = (m0xxs[i] * m1zxs[i]) + (m0yxs[i] * m1zys[i]) + (m0zxs[i] * m1zzs[i]) + (m0wxs[i] * m1zws[i]);
		}

		for (int i = 0; i < N; i++)
		{
			outMzys[i] = (m0xys[i] * m1zxs[i]) + (m0yys[i] * m1zys[i]) + (m0zys[i] * m1zzs[i]) + (m0wys[i] * m1zws[i]);
		}

		for (int i = 0; i < N; i++)
		{
			outMzzs[i] = (m0xzs[i] * m1zxs[i]) + (m0yzs[i] * m1zys[i]) + (m0zzs[i] * m1zzs[i]) + (m0wzs[i] * m1zws[i]);
		}

		for (int i = 0; i < N; i++)
		{
			outMzws[i] = (m0xws[i] * m1zxs[i]) + (m0yws[i] * m1zys[i]) + (m0zws[i] * m1zzs[i]) + (m0wws[i] * m1zws[i]);
		}

		for (int i = 0; i < N; i++)
		{
			outMwxs[i] = (m0xxs[i] * m1wxs[i]) + (m0yxs[i] * m1wys[i]) + (m0zxs[i] * m1wzs[i]) + (m0wxs[i] * m1wws[i]);
		}

		for (int i = 0; i < N; i++)
		{
			outMwys[i] = (m0xys[i] * m1wxs[i]) + (m0yys[i] * m1wys[i]) + (m0zys[i] * m1wzs[i]) + (m0wys[i] * m1wws[i]);
		}

		for (int i = 0; i < N; i++)
		{
			outMwzs[i] = (m0xzs[i] * m1wxs[i]) + (m0yzs[i] * m1wys[i]) + (m0zzs[i] * m1wzs[i]) + (m0wzs[i] * m1wws[i]);
		}

		for (int i = 0; i < N; i++)
		{
			outMwws[i] = (m0xws[i] * m1wxs[i]) + (m0yws[i] * m1wys[i]) + (m0zws[i] * m1wzs[i]) + (m0wws[i] * m1wws[i]);
		}
	}
}

// Camera globals.
namespace
{
	Double3 g_horizonNdcPoint; // For horizon reflections.

	Matrix4d g_viewMatrix;
	Matrix4d g_projMatrix;

	Matrix4d g_viewProjMatrix;
	double g_viewProjMatrixXX[TYPICAL_LOOP_UNROLL];
	double g_viewProjMatrixXY[TYPICAL_LOOP_UNROLL];
	double g_viewProjMatrixXZ[TYPICAL_LOOP_UNROLL];
	double g_viewProjMatrixXW[TYPICAL_LOOP_UNROLL];
	double g_viewProjMatrixYX[TYPICAL_LOOP_UNROLL];
	double g_viewProjMatrixYY[TYPICAL_LOOP_UNROLL];
	double g_viewProjMatrixYZ[TYPICAL_LOOP_UNROLL];
	double g_viewProjMatrixYW[TYPICAL_LOOP_UNROLL];
	double g_viewProjMatrixZX[TYPICAL_LOOP_UNROLL];
	double g_viewProjMatrixZY[TYPICAL_LOOP_UNROLL];
	double g_viewProjMatrixZZ[TYPICAL_LOOP_UNROLL];
	double g_viewProjMatrixZW[TYPICAL_LOOP_UNROLL];
	double g_viewProjMatrixWX[TYPICAL_LOOP_UNROLL];
	double g_viewProjMatrixWY[TYPICAL_LOOP_UNROLL];
	double g_viewProjMatrixWZ[TYPICAL_LOOP_UNROLL];
	double g_viewProjMatrixWW[TYPICAL_LOOP_UNROLL];

	Matrix4d g_invViewMatrix;
	double g_invViewMatrixXX[TYPICAL_LOOP_UNROLL];
	double g_invViewMatrixXY[TYPICAL_LOOP_UNROLL];
	double g_invViewMatrixXZ[TYPICAL_LOOP_UNROLL];
	double g_invViewMatrixXW[TYPICAL_LOOP_UNROLL];
	double g_invViewMatrixYX[TYPICAL_LOOP_UNROLL];
	double g_invViewMatrixYY[TYPICAL_LOOP_UNROLL];
	double g_invViewMatrixYZ[TYPICAL_LOOP_UNROLL];
	double g_invViewMatrixYW[TYPICAL_LOOP_UNROLL];
	double g_invViewMatrixZX[TYPICAL_LOOP_UNROLL];
	double g_invViewMatrixZY[TYPICAL_LOOP_UNROLL];
	double g_invViewMatrixZZ[TYPICAL_LOOP_UNROLL];
	double g_invViewMatrixZW[TYPICAL_LOOP_UNROLL];
	double g_invViewMatrixWX[TYPICAL_LOOP_UNROLL];
	double g_invViewMatrixWY[TYPICAL_LOOP_UNROLL];
	double g_invViewMatrixWZ[TYPICAL_LOOP_UNROLL];
	double g_invViewMatrixWW[TYPICAL_LOOP_UNROLL];

	Matrix4d g_invProjMatrix;
	double g_invProjMatrixXX[TYPICAL_LOOP_UNROLL];
	double g_invProjMatrixXY[TYPICAL_LOOP_UNROLL];
	double g_invProjMatrixXZ[TYPICAL_LOOP_UNROLL];
	double g_invProjMatrixXW[TYPICAL_LOOP_UNROLL];
	double g_invProjMatrixYX[TYPICAL_LOOP_UNROLL];
	double g_invProjMatrixYY[TYPICAL_LOOP_UNROLL];
	double g_invProjMatrixYZ[TYPICAL_LOOP_UNROLL];
	double g_invProjMatrixYW[TYPICAL_LOOP_UNROLL];
	double g_invProjMatrixZX[TYPICAL_LOOP_UNROLL];
	double g_invProjMatrixZY[TYPICAL_LOOP_UNROLL];
	double g_invProjMatrixZZ[TYPICAL_LOOP_UNROLL];
	double g_invProjMatrixZW[TYPICAL_LOOP_UNROLL];
	double g_invProjMatrixWX[TYPICAL_LOOP_UNROLL];
	double g_invProjMatrixWY[TYPICAL_LOOP_UNROLL];
	double g_invProjMatrixWZ[TYPICAL_LOOP_UNROLL];
	double g_invProjMatrixWW[TYPICAL_LOOP_UNROLL];

	void PopulateCameraGlobals(const RenderCamera &camera)
	{
		g_horizonNdcPoint = camera.horizonNdcPoint;
		g_viewMatrix = camera.viewMatrix;
		g_projMatrix = camera.projectionMatrix;
		g_viewProjMatrix = camera.projectionMatrix * camera.viewMatrix;
		g_invViewMatrix = camera.inverseViewMatrix;
		g_invProjMatrix = camera.inverseProjectionMatrix;

		for (int i = 0; i < TYPICAL_LOOP_UNROLL; i++)
		{
			g_viewProjMatrixXX[i] = g_viewProjMatrix.x.x;
			g_viewProjMatrixXY[i] = g_viewProjMatrix.x.y;
			g_viewProjMatrixXZ[i] = g_viewProjMatrix.x.z;
			g_viewProjMatrixXW[i] = g_viewProjMatrix.x.w;
			g_viewProjMatrixYX[i] = g_viewProjMatrix.y.x;
			g_viewProjMatrixYY[i] = g_viewProjMatrix.y.y;
			g_viewProjMatrixYZ[i] = g_viewProjMatrix.y.z;
			g_viewProjMatrixYW[i] = g_viewProjMatrix.y.w;
			g_viewProjMatrixZX[i] = g_viewProjMatrix.z.x;
			g_viewProjMatrixZY[i] = g_viewProjMatrix.z.y;
			g_viewProjMatrixZZ[i] = g_viewProjMatrix.z.z;
			g_viewProjMatrixZW[i] = g_viewProjMatrix.z.w;
			g_viewProjMatrixWX[i] = g_viewProjMatrix.w.x;
			g_viewProjMatrixWY[i] = g_viewProjMatrix.w.y;
			g_viewProjMatrixWZ[i] = g_viewProjMatrix.w.z;
			g_viewProjMatrixWW[i] = g_viewProjMatrix.w.w;

			g_invViewMatrixXX[i] = g_invViewMatrix.x.x;
			g_invViewMatrixXY[i] = g_invViewMatrix.x.y;
			g_invViewMatrixXZ[i] = g_invViewMatrix.x.z;
			g_invViewMatrixXW[i] = g_invViewMatrix.x.w;
			g_invViewMatrixYX[i] = g_invViewMatrix.y.x;
			g_invViewMatrixYY[i] = g_invViewMatrix.y.y;
			g_invViewMatrixYZ[i] = g_invViewMatrix.y.z;
			g_invViewMatrixYW[i] = g_invViewMatrix.y.w;
			g_invViewMatrixZX[i] = g_invViewMatrix.z.x;
			g_invViewMatrixZY[i] = g_invViewMatrix.z.y;
			g_invViewMatrixZZ[i] = g_invViewMatrix.z.z;
			g_invViewMatrixZW[i] = g_invViewMatrix.z.w;
			g_invViewMatrixWX[i] = g_invViewMatrix.w.x;
			g_invViewMatrixWY[i] = g_invViewMatrix.w.y;
			g_invViewMatrixWZ[i] = g_invViewMatrix.w.z;
			g_invViewMatrixWW[i] = g_invViewMatrix.w.w;

			g_invProjMatrixXX[i] = g_invProjMatrix.x.x;
			g_invProjMatrixXY[i] = g_invProjMatrix.x.y;
			g_invProjMatrixXZ[i] = g_invProjMatrix.x.z;
			g_invProjMatrixXW[i] = g_invProjMatrix.x.w;
			g_invProjMatrixYX[i] = g_invProjMatrix.y.x;
			g_invProjMatrixYY[i] = g_invProjMatrix.y.y;
			g_invProjMatrixYZ[i] = g_invProjMatrix.y.z;
			g_invProjMatrixYW[i] = g_invProjMatrix.y.w;
			g_invProjMatrixZX[i] = g_invProjMatrix.z.x;
			g_invProjMatrixZY[i] = g_invProjMatrix.z.y;
			g_invProjMatrixZZ[i] = g_invProjMatrix.z.z;
			g_invProjMatrixZW[i] = g_invProjMatrix.z.w;
			g_invProjMatrixWX[i] = g_invProjMatrix.w.x;
			g_invProjMatrixWY[i] = g_invProjMatrix.w.y;
			g_invProjMatrixWZ[i] = g_invProjMatrix.w.z;
			g_invProjMatrixWW[i] = g_invProjMatrix.w.w;
		}
	}
}

// Draw call globals.
namespace
{
	struct DrawCallCache
	{
		const SoftwareRenderer::VertexBuffer *vertexBuffer;
		const SoftwareRenderer::AttributeBuffer *texCoordBuffer;
		const SoftwareRenderer::IndexBuffer *indexBuffer;
		ObjectTextureID textureID0;
		ObjectTextureID textureID1;
		RenderLightingType lightingType;
		double meshLightPercent;
		const SoftwareRenderer::Light *lightPtrArray[RenderLightIdList::MAX_LIGHTS];
		int lightCount;
		VertexShaderType vertexShaderType;
		PixelShaderType pixelShaderType;
		double pixelShaderParam0;
		bool enableDepthRead;
		bool enableDepthWrite;
	};

	// Transform for the mesh to be processed with.
	struct TransformCache
	{
		double translationMatrixXX;
		double translationMatrixXY;
		double translationMatrixXZ;
		double translationMatrixXW;
		double translationMatrixYX;
		double translationMatrixYY;
		double translationMatrixYZ;
		double translationMatrixYW;
		double translationMatrixZX;
		double translationMatrixZY;
		double translationMatrixZZ;
		double translationMatrixZW;
		double translationMatrixWX;
		double translationMatrixWY;
		double translationMatrixWZ;
		double translationMatrixWW;
		double rotationMatrixXX;
		double rotationMatrixXY;
		double rotationMatrixXZ;
		double rotationMatrixXW;
		double rotationMatrixYX;
		double rotationMatrixYY;
		double rotationMatrixYZ;
		double rotationMatrixYW;
		double rotationMatrixZX;
		double rotationMatrixZY;
		double rotationMatrixZZ;
		double rotationMatrixZW;
		double rotationMatrixWX;
		double rotationMatrixWY;
		double rotationMatrixWZ;
		double rotationMatrixWW;
		double scaleMatrixXX;
		double scaleMatrixXY;
		double scaleMatrixXZ;
		double scaleMatrixXW;
		double scaleMatrixYX;
		double scaleMatrixYY;
		double scaleMatrixYZ;
		double scaleMatrixYW;
		double scaleMatrixZX;
		double scaleMatrixZY;
		double scaleMatrixZZ;
		double scaleMatrixZW;
		double scaleMatrixWX;
		double scaleMatrixWY;
		double scaleMatrixWZ;
		double scaleMatrixWW;
		double modelViewProjMatrixXX;
		double modelViewProjMatrixXY;
		double modelViewProjMatrixXZ;
		double modelViewProjMatrixXW;
		double modelViewProjMatrixYX;
		double modelViewProjMatrixYY;
		double modelViewProjMatrixYZ;
		double modelViewProjMatrixYW;
		double modelViewProjMatrixZX;
		double modelViewProjMatrixZY;
		double modelViewProjMatrixZZ;
		double modelViewProjMatrixZW;
		double modelViewProjMatrixWX;
		double modelViewProjMatrixWY;
		double modelViewProjMatrixWZ;
		double modelViewProjMatrixWW;
		double preScaleTranslationX;
		double preScaleTranslationY;
		double preScaleTranslationZ;
	};

	int g_totalDrawCallCount = 0;

	void PopulateDrawCallGlobals(int totalDrawCallCount)
	{
		g_totalDrawCallCount = totalDrawCallCount;
	}

	void PopulateMeshTransform(TransformCache &cache, const RenderTransform &transform)
	{
		cache.translationMatrixXX = transform.translation.x.x;
		cache.translationMatrixXY = transform.translation.x.y;
		cache.translationMatrixXZ = transform.translation.x.z;
		cache.translationMatrixXW = transform.translation.x.w;
		cache.translationMatrixYX = transform.translation.y.x;
		cache.translationMatrixYY = transform.translation.y.y;
		cache.translationMatrixYZ = transform.translation.y.z;
		cache.translationMatrixYW = transform.translation.y.w;
		cache.translationMatrixZX = transform.translation.z.x;
		cache.translationMatrixZY = transform.translation.z.y;
		cache.translationMatrixZZ = transform.translation.z.z;
		cache.translationMatrixZW = transform.translation.z.w;
		cache.translationMatrixWX = transform.translation.w.x;
		cache.translationMatrixWY = transform.translation.w.y;
		cache.translationMatrixWZ = transform.translation.w.z;
		cache.translationMatrixWW = transform.translation.w.w;
		cache.rotationMatrixXX = transform.rotation.x.x;
		cache.rotationMatrixXY = transform.rotation.x.y;
		cache.rotationMatrixXZ = transform.rotation.x.z;
		cache.rotationMatrixXW = transform.rotation.x.w;
		cache.rotationMatrixYX = transform.rotation.y.x;
		cache.rotationMatrixYY = transform.rotation.y.y;
		cache.rotationMatrixYZ = transform.rotation.y.z;
		cache.rotationMatrixYW = transform.rotation.y.w;
		cache.rotationMatrixZX = transform.rotation.z.x;
		cache.rotationMatrixZY = transform.rotation.z.y;
		cache.rotationMatrixZZ = transform.rotation.z.z;
		cache.rotationMatrixZW = transform.rotation.z.w;
		cache.rotationMatrixWX = transform.rotation.w.x;
		cache.rotationMatrixWY = transform.rotation.w.y;
		cache.rotationMatrixWZ = transform.rotation.w.z;
		cache.rotationMatrixWW = transform.rotation.w.w;
		cache.scaleMatrixXX = transform.scale.x.x;
		cache.scaleMatrixXY = transform.scale.x.y;
		cache.scaleMatrixXZ = transform.scale.x.z;
		cache.scaleMatrixXW = transform.scale.x.w;
		cache.scaleMatrixYX = transform.scale.y.x;
		cache.scaleMatrixYY = transform.scale.y.y;
		cache.scaleMatrixYZ = transform.scale.y.z;
		cache.scaleMatrixYW = transform.scale.y.w;
		cache.scaleMatrixZX = transform.scale.z.x;
		cache.scaleMatrixZY = transform.scale.z.y;
		cache.scaleMatrixZZ = transform.scale.z.z;
		cache.scaleMatrixZW = transform.scale.z.w;
		cache.scaleMatrixWX = transform.scale.w.x;
		cache.scaleMatrixWY = transform.scale.w.y;
		cache.scaleMatrixWZ = transform.scale.w.z;
		cache.scaleMatrixWW = transform.scale.w.w;
		// Do model-view-projection matrix in the bulk processing loop.
	}
}

// Rasterization utils.
namespace
{
	struct RasterizerTriangle
	{
		// The rasterizer prefers vertices in AoS layout.
		double clip0X, clip0Y, clip0Z, clip0W;
		double clip1X, clip1Y, clip1Z, clip1W;
		double clip2X, clip2Y, clip2Z, clip2W;
		double clip0WRecip;
		double clip1WRecip;
		double clip2WRecip;
		double ndc0X, ndc0Y, ndc0Z;
		double ndc1X, ndc1Y, ndc1Z;
		double ndc2X, ndc2Y, ndc2Z;
		double screenSpace0X, screenSpace0Y;
		double screenSpace1X, screenSpace1Y;
		double screenSpace2X, screenSpace2Y;
		double screenSpace01X, screenSpace01Y;
		double screenSpace12X, screenSpace12Y;
		double screenSpace20X, screenSpace20Y;
		double screenSpace01PerpX, screenSpace01PerpY;
		double screenSpace12PerpX, screenSpace12PerpY;
		double screenSpace20PerpX, screenSpace20PerpY;
		double uv0X, uv0Y;
		double uv1X, uv1Y;
		double uv2X, uv2Y;
		double uv0XDivW, uv0YDivW;
		double uv1XDivW, uv1YDivW;
		double uv2XDivW, uv2YDivW;
	};

	double NdcXToScreenSpace(double ndcX, double frameWidth)
	{
		return (0.50 + (ndcX * 0.50)) * frameWidth;
	}

	double NdcYToScreenSpace(double ndcY, double frameHeight)
	{
		return (0.50 - (ndcY * 0.50)) * frameHeight;
	}

	// Helper function for the dot product X's or Y's used to see if a screen space point is inside the triangle.
	void GetScreenSpacePointHalfSpaceComponents(double pointComponent, double plane0PointComponent,
		double plane1PointComponent, double plane2PointComponent, double plane0NormalComponent,
		double plane1NormalComponent, double plane2NormalComponent, double *__restrict outDot0Component,
		double *__restrict outDot1Component, double *__restrict outDot2Component)
	{
		const double point0Diff = pointComponent - plane0PointComponent;
		const double point1Diff = pointComponent - plane1PointComponent;
		const double point2Diff = pointComponent - plane2PointComponent;
		*outDot0Component = point0Diff * plane0NormalComponent;
		*outDot1Component = point1Diff * plane1NormalComponent;
		*outDot2Component = point2Diff * plane2NormalComponent;
	}

	// Bin dimensions vary with frame buffer resolution for better thread balancing.
	constexpr int RASTERIZER_BIN_MIN_WIDTH = 64; // For low resolutions (<720p).
	constexpr int RASTERIZER_BIN_MAX_WIDTH = 512; // For high resolutions (>2160p).
	constexpr int RASTERIZER_BIN_MIN_HEIGHT = RASTERIZER_BIN_MIN_WIDTH;
	constexpr int RASTERIZER_BIN_MAX_HEIGHT = RASTERIZER_BIN_MAX_WIDTH;
	constexpr int RASTERIZER_TYPICAL_BINS_PER_FRAME_BUFFER_WIDTH = 16;
	constexpr int RASTERIZER_TYPICAL_BINS_PER_FRAME_BUFFER_HEIGHT = 9;
	static_assert(MathUtils::isPowerOf2(RASTERIZER_BIN_MIN_WIDTH));
	static_assert(MathUtils::isPowerOf2(RASTERIZER_BIN_MAX_WIDTH));
	static_assert(MathUtils::isPowerOf2(RASTERIZER_BIN_MIN_HEIGHT));
	static_assert(MathUtils::isPowerOf2(RASTERIZER_BIN_MAX_HEIGHT));

	int GetRasterizerBinWidth(int frameBufferWidth)
	{
		const int estimatedBinWidth = frameBufferWidth / RASTERIZER_TYPICAL_BINS_PER_FRAME_BUFFER_WIDTH;
		const int powerOfTwoBinWidth = MathUtils::roundToGreaterPowerOf2(estimatedBinWidth);
		return std::clamp(powerOfTwoBinWidth, RASTERIZER_BIN_MIN_WIDTH, RASTERIZER_BIN_MAX_WIDTH);
	}

	int GetRasterizerBinHeight(int frameBufferHeight)
	{
		const int estimatedBinHeight = frameBufferHeight / RASTERIZER_TYPICAL_BINS_PER_FRAME_BUFFER_HEIGHT;
		const int powerOfTwoBinHeight = MathUtils::roundToGreaterPowerOf2(estimatedBinHeight);
		return std::clamp(powerOfTwoBinHeight, RASTERIZER_BIN_MIN_HEIGHT, RASTERIZER_BIN_MAX_HEIGHT);
	}

	int GetRasterizerBinCountX(int frameBufferWidth, int binWidth)
	{
		return 1 + (frameBufferWidth / binWidth);
	}

	int GetRasterizerBinCountY(int frameBufferHeight, int binHeight)
	{
		return 1 + (frameBufferHeight / binHeight);
	}

	int GetRasterizerBinX(int frameBufferPixelX, int binWidth)
	{
		return frameBufferPixelX / binWidth;
	}

	int GetRasterizerBinY(int frameBufferPixelY, int binHeight)
	{
		return frameBufferPixelY / binHeight;
	}

	int GetRasterizerBinPixelXInclusive(int frameBufferPixelX, int binWidth)
	{
		return frameBufferPixelX % binWidth;
	}

	int GetRasterizerBinPixelXExclusive(int frameBufferPixelX, int binWidth)
	{
		const int modulo = frameBufferPixelX % binWidth;
		return (modulo != 0) ? modulo : binWidth;
	}

	int GetRasterizerBinPixelYInclusive(int frameBufferPixelY, int binHeight)
	{
		return frameBufferPixelY % binHeight;
	}

	int GetRasterizerBinPixelYExclusive(int frameBufferPixelY, int binHeight)
	{
		const int modulo = frameBufferPixelY % binHeight;
		return (modulo != 0) ? modulo : binHeight;
	}

	int GetFrameBufferPixelX(int binX, int binPixelX, int binWidth)
	{
		return (binX * binWidth) + binPixelX;
	}

	int GetFrameBufferPixelY(int binY, int binPixelY, int binHeight)
	{
		return (binY * binHeight) + binPixelY;
	}
}

// Frame buffer globals.
namespace
{
	int g_frameBufferWidth;
	int g_frameBufferHeight;
	int g_frameBufferPixelCount;
	double g_frameBufferWidthReal;
	double g_frameBufferHeightReal;
	double g_frameBufferWidthRealRecip;
	double g_frameBufferHeightRealRecip;
	int g_ditherBufferDepth;
	DitheringMode g_ditheringMode;
	uint8_t *g_paletteIndexBuffer;
	double *g_depthBuffer;
	const bool *g_ditherBuffer;
	uint32_t *g_colorBuffer;
	SoftwareRenderer::ObjectTexturePool *g_objectTextures;

	void PopulateRasterizerGlobals(int frameBufferWidth, int frameBufferHeight, uint8_t *paletteIndexBuffer, double *depthBuffer,
		const bool *ditherBuffer, int ditherBufferDepth, DitheringMode ditheringMode, uint32_t *colorBuffer,
		SoftwareRenderer::ObjectTexturePool *objectTextures)
	{
		g_frameBufferWidth = frameBufferWidth;
		g_frameBufferHeight = frameBufferHeight;
		g_frameBufferPixelCount = frameBufferWidth * frameBufferHeight;
		g_frameBufferWidthReal = static_cast<double>(frameBufferWidth);
		g_frameBufferHeightReal = static_cast<double>(frameBufferHeight);
		g_frameBufferWidthRealRecip = 1.0 / g_frameBufferWidthReal;
		g_frameBufferHeightRealRecip = 1.0 / g_frameBufferHeightReal;
		g_ditherBufferDepth = ditherBufferDepth;
		g_ditheringMode = ditheringMode;
		g_paletteIndexBuffer = paletteIndexBuffer;
		g_depthBuffer = depthBuffer;
		g_ditherBuffer = ditherBuffer;
		g_colorBuffer = colorBuffer;
		g_objectTextures = objectTextures;
	}

	// For measuring overdraw.
	std::atomic<int> g_totalCoverageTests = 0;
	std::atomic<int> g_totalDepthTests = 0;
	std::atomic<int> g_totalColorWrites = 0;

	void ClearFrameBuffers()
	{
		// Don't need to clear color buffers as long as there's always a sky mesh.
		//std::fill(g_paletteIndexBuffer, g_paletteIndexBuffer + g_frameBufferPixelCount, 0);
		std::fill(g_depthBuffer, g_depthBuffer + g_frameBufferPixelCount, std::numeric_limits<double>::infinity());
		//std::fill(g_colorBuffer, g_colorBuffer + g_frameBufferPixelCount, 0);
		g_totalCoverageTests = 0;
		g_totalDepthTests = 0;
		g_totalColorWrites = 0;
	}

	void CreateDitherBuffer(Buffer3D<bool> &ditherBuffer, int width, int height, DitheringMode ditheringMode)
	{
		if (ditheringMode == DitheringMode::Classic)
		{
			// Original game: 2x2, top left + bottom right are darkened.
			ditherBuffer.init(width, height, 1);

			bool *ditherPixels = ditherBuffer.begin();
			for (int y = 0; y < height; y++)
			{
				for (int x = 0; x < width; x++)
				{
					const bool shouldDither = ((x + y) & 0x1) == 0;
					const int index = x + (y * width);
					ditherPixels[index] = shouldDither;
				}
			}
		}
		else if (ditheringMode == DitheringMode::Modern)
		{
			// Modern 2x2, four levels of dither depending on percent between two light levels.
			ditherBuffer.init(width, height, DITHERING_MODERN_MASK_COUNT);
			static_assert(DITHERING_MODERN_MASK_COUNT == 4);

			bool *ditherPixels = ditherBuffer.begin();
			for (int y = 0; y < height; y++)
			{
				for (int x = 0; x < width; x++)
				{
					const bool shouldDither0 = (((x + y) & 0x1) == 0) || (((x % 2) == 1) && ((y % 2) == 0)); // Top left, bottom right, top right
					const bool shouldDither1 = ((x + y) & 0x1) == 0; // Top left + bottom right
					const bool shouldDither2 = ((x % 2) == 0) && ((y % 2) == 0); // Top left
					const bool shouldDither3 = false;
					const int index0 = x + (y * width);
					const int index1 = x + (y * width) + (1 * width * height);
					const int index2 = x + (y * width) + (2 * width * height);
					const int index3 = x + (y * width) + (3 * width * height);
					ditherPixels[index0] = shouldDither0;
					ditherPixels[index1] = shouldDither1;
					ditherPixels[index2] = shouldDither2;
					ditherPixels[index3] = shouldDither3;
				}
			}
		}
		else
		{
			ditherBuffer.clear();
		}
	}
}

// Vertex shaders.
namespace
{
	template<int N>
	void VertexShader_BasicN(const TransformCache &__restrict transformCache, const double *__restrict vertexXs, const double *__restrict vertexYs,
		const double *__restrict vertexZs, const double *__restrict vertexWs, double *__restrict outVertexXs, double *__restrict outVertexYs,
		double *__restrict outVertexZs, double *__restrict outVertexWs)
	{
		// Read in each mesh's transforms.
		double modelViewProjMatrixXXs[N];
		double modelViewProjMatrixXYs[N];
		double modelViewProjMatrixXZs[N];
		double modelViewProjMatrixXWs[N];
		double modelViewProjMatrixYXs[N];
		double modelViewProjMatrixYYs[N];
		double modelViewProjMatrixYZs[N];
		double modelViewProjMatrixYWs[N];
		double modelViewProjMatrixZXs[N];
		double modelViewProjMatrixZYs[N];
		double modelViewProjMatrixZZs[N];
		double modelViewProjMatrixZWs[N];
		double modelViewProjMatrixWXs[N];
		double modelViewProjMatrixWYs[N];
		double modelViewProjMatrixWZs[N];
		double modelViewProjMatrixWWs[N];
		for (int i = 0; i < N; i++)
		{
			// @todo: this isn't taking meshIndex anymore
			modelViewProjMatrixXXs[i] = transformCache.modelViewProjMatrixXX;
			modelViewProjMatrixXYs[i] = transformCache.modelViewProjMatrixXY;
			modelViewProjMatrixXZs[i] = transformCache.modelViewProjMatrixXZ;
			modelViewProjMatrixXWs[i] = transformCache.modelViewProjMatrixXW;
			modelViewProjMatrixYXs[i] = transformCache.modelViewProjMatrixYX;
			modelViewProjMatrixYYs[i] = transformCache.modelViewProjMatrixYY;
			modelViewProjMatrixYZs[i] = transformCache.modelViewProjMatrixYZ;
			modelViewProjMatrixYWs[i] = transformCache.modelViewProjMatrixYW;
			modelViewProjMatrixZXs[i] = transformCache.modelViewProjMatrixZX;
			modelViewProjMatrixZYs[i] = transformCache.modelViewProjMatrixZY;
			modelViewProjMatrixZZs[i] = transformCache.modelViewProjMatrixZZ;
			modelViewProjMatrixZWs[i] = transformCache.modelViewProjMatrixZW;
			modelViewProjMatrixWXs[i] = transformCache.modelViewProjMatrixWX;
			modelViewProjMatrixWYs[i] = transformCache.modelViewProjMatrixWY;
			modelViewProjMatrixWZs[i] = transformCache.modelViewProjMatrixWZ;
			modelViewProjMatrixWWs[i] = transformCache.modelViewProjMatrixWW;
		}

		// Apply model-view-projection matrix.
		Matrix4_MultiplyVectorN<N>(
			modelViewProjMatrixXXs, modelViewProjMatrixXYs, modelViewProjMatrixXZs, modelViewProjMatrixXWs,
			modelViewProjMatrixYXs, modelViewProjMatrixYYs, modelViewProjMatrixYZs, modelViewProjMatrixYWs,
			modelViewProjMatrixZXs, modelViewProjMatrixZYs, modelViewProjMatrixZZs, modelViewProjMatrixZWs,
			modelViewProjMatrixWXs, modelViewProjMatrixWYs, modelViewProjMatrixWZs, modelViewProjMatrixWWs,
			vertexXs, vertexYs, vertexZs, vertexWs,
			outVertexXs, outVertexYs, outVertexZs, outVertexWs);
	}

	template<int N>
	void VertexShader_RaisingDoorN(const TransformCache &__restrict transformCache, const double *__restrict vertexXs, const double *__restrict vertexYs,
		const double *__restrict vertexZs, const double *__restrict vertexWs, double *__restrict outVertexXs, double *__restrict outVertexYs,
		double *__restrict outVertexZs, double *__restrict outVertexWs)
	{
		// Read in each mesh's transforms.
		double preScaleTranslationXs[N];
		double preScaleTranslationYs[N];
		double preScaleTranslationZs[N];
		double translationMatrixXXs[N];
		double translationMatrixXYs[N];
		double translationMatrixXZs[N];
		double translationMatrixXWs[N];
		double translationMatrixYXs[N];
		double translationMatrixYYs[N];
		double translationMatrixYZs[N];
		double translationMatrixYWs[N];
		double translationMatrixZXs[N];
		double translationMatrixZYs[N];
		double translationMatrixZZs[N];
		double translationMatrixZWs[N];
		double translationMatrixWXs[N];
		double translationMatrixWYs[N];
		double translationMatrixWZs[N];
		double translationMatrixWWs[N];
		double rotationMatrixXXs[N];
		double rotationMatrixXYs[N];
		double rotationMatrixXZs[N];
		double rotationMatrixXWs[N];
		double rotationMatrixYXs[N];
		double rotationMatrixYYs[N];
		double rotationMatrixYZs[N];
		double rotationMatrixYWs[N];
		double rotationMatrixZXs[N];
		double rotationMatrixZYs[N];
		double rotationMatrixZZs[N];
		double rotationMatrixZWs[N];
		double rotationMatrixWXs[N];
		double rotationMatrixWYs[N];
		double rotationMatrixWZs[N];
		double rotationMatrixWWs[N];
		double scaleMatrixXXs[N];
		double scaleMatrixXYs[N];
		double scaleMatrixXZs[N];
		double scaleMatrixXWs[N];
		double scaleMatrixYXs[N];
		double scaleMatrixYYs[N];
		double scaleMatrixYZs[N];
		double scaleMatrixYWs[N];
		double scaleMatrixZXs[N];
		double scaleMatrixZYs[N];
		double scaleMatrixZZs[N];
		double scaleMatrixZWs[N];
		double scaleMatrixWXs[N];
		double scaleMatrixWYs[N];
		double scaleMatrixWZs[N];
		double scaleMatrixWWs[N];
		for (int i = 0; i < N; i++)
		{
			// @todo: this isn't taking meshIndex anymore
			preScaleTranslationXs[i] = transformCache.preScaleTranslationX;
			preScaleTranslationYs[i] = transformCache.preScaleTranslationY;
			preScaleTranslationZs[i] = transformCache.preScaleTranslationZ;
			translationMatrixXXs[i] = transformCache.translationMatrixXX;
			translationMatrixXYs[i] = transformCache.translationMatrixXY;
			translationMatrixXZs[i] = transformCache.translationMatrixXZ;
			translationMatrixXWs[i] = transformCache.translationMatrixXW;
			translationMatrixYXs[i] = transformCache.translationMatrixYX;
			translationMatrixYYs[i] = transformCache.translationMatrixYY;
			translationMatrixYZs[i] = transformCache.translationMatrixYZ;
			translationMatrixYWs[i] = transformCache.translationMatrixYW;
			translationMatrixZXs[i] = transformCache.translationMatrixZX;
			translationMatrixZYs[i] = transformCache.translationMatrixZY;
			translationMatrixZZs[i] = transformCache.translationMatrixZZ;
			translationMatrixZWs[i] = transformCache.translationMatrixZW;
			translationMatrixWXs[i] = transformCache.translationMatrixWX;
			translationMatrixWYs[i] = transformCache.translationMatrixWY;
			translationMatrixWZs[i] = transformCache.translationMatrixWZ;
			translationMatrixWWs[i] = transformCache.translationMatrixWW;
			rotationMatrixXXs[i] = transformCache.rotationMatrixXX;
			rotationMatrixXYs[i] = transformCache.rotationMatrixXY;
			rotationMatrixXZs[i] = transformCache.rotationMatrixXZ;
			rotationMatrixXWs[i] = transformCache.rotationMatrixXW;
			rotationMatrixYXs[i] = transformCache.rotationMatrixYX;
			rotationMatrixYYs[i] = transformCache.rotationMatrixYY;
			rotationMatrixYZs[i] = transformCache.rotationMatrixYZ;
			rotationMatrixYWs[i] = transformCache.rotationMatrixYW;
			rotationMatrixZXs[i] = transformCache.rotationMatrixZX;
			rotationMatrixZYs[i] = transformCache.rotationMatrixZY;
			rotationMatrixZZs[i] = transformCache.rotationMatrixZZ;
			rotationMatrixZWs[i] = transformCache.rotationMatrixZW;
			rotationMatrixWXs[i] = transformCache.rotationMatrixWX;
			rotationMatrixWYs[i] = transformCache.rotationMatrixWY;
			rotationMatrixWZs[i] = transformCache.rotationMatrixWZ;
			rotationMatrixWWs[i] = transformCache.rotationMatrixWW;
			scaleMatrixXXs[i] = transformCache.scaleMatrixXX;
			scaleMatrixXYs[i] = transformCache.scaleMatrixXY;
			scaleMatrixXZs[i] = transformCache.scaleMatrixXZ;
			scaleMatrixXWs[i] = transformCache.scaleMatrixXW;
			scaleMatrixYXs[i] = transformCache.scaleMatrixYX;
			scaleMatrixYYs[i] = transformCache.scaleMatrixYY;
			scaleMatrixYZs[i] = transformCache.scaleMatrixYZ;
			scaleMatrixYWs[i] = transformCache.scaleMatrixYW;
			scaleMatrixZXs[i] = transformCache.scaleMatrixZX;
			scaleMatrixZYs[i] = transformCache.scaleMatrixZY;
			scaleMatrixZZs[i] = transformCache.scaleMatrixZZ;
			scaleMatrixZWs[i] = transformCache.scaleMatrixZW;
			scaleMatrixWXs[i] = transformCache.scaleMatrixWX;
			scaleMatrixWYs[i] = transformCache.scaleMatrixWY;
			scaleMatrixWZs[i] = transformCache.scaleMatrixWZ;
			scaleMatrixWWs[i] = transformCache.scaleMatrixWW;
		}

		// Translate down so floor vertices go underground and ceiling is at y=0.
		const double preScaleTranslationWs[N] = { 0.0 };
		double vertexWithPreScaleTranslationXs[N];
		double vertexWithPreScaleTranslationYs[N];
		double vertexWithPreScaleTranslationZs[N];
		double vertexWithPreScaleTranslationWs[N];
		Double4_AddN<N>(vertexXs, vertexYs, vertexZs, vertexWs,
			preScaleTranslationXs, preScaleTranslationYs, preScaleTranslationZs, preScaleTranslationWs,
			vertexWithPreScaleTranslationXs, vertexWithPreScaleTranslationYs, vertexWithPreScaleTranslationZs, vertexWithPreScaleTranslationWs);

		// Shrink towards y=0 depending on anim percent and door min visible amount.
		double scaledVertexXs[N] = { 0.0 };
		double scaledVertexYs[N] = { 0.0 };
		double scaledVertexZs[N] = { 0.0 };
		double scaledVertexWs[N] = { 0.0 };
		Matrix4_MultiplyVectorN<N>(
			scaleMatrixXXs, scaleMatrixXYs, scaleMatrixXZs, scaleMatrixXWs,
			scaleMatrixYXs, scaleMatrixYYs, scaleMatrixYZs, scaleMatrixYWs,
			scaleMatrixZXs, scaleMatrixZYs, scaleMatrixZZs, scaleMatrixZWs,
			scaleMatrixWXs, scaleMatrixWYs, scaleMatrixWZs, scaleMatrixWWs,
			vertexWithPreScaleTranslationXs, vertexWithPreScaleTranslationYs, vertexWithPreScaleTranslationZs, vertexWithPreScaleTranslationWs,
			scaledVertexXs, scaledVertexYs, scaledVertexZs, scaledVertexWs);

		// Translate up to new model space Y position.
		double resultVertexXs[N];
		double resultVertexYs[N];
		double resultVertexZs[N];
		double resultVertexWs[N];
		Double4_SubtractN<N>(scaledVertexXs, scaledVertexYs, scaledVertexZs, scaledVertexWs,
			preScaleTranslationXs, preScaleTranslationYs, preScaleTranslationZs, preScaleTranslationWs,
			resultVertexXs, resultVertexYs, resultVertexZs, resultVertexWs);

		// Apply rotation matrix.
		double rotatedResultVertexXs[N] = { 0.0 };
		double rotatedResultVertexYs[N] = { 0.0 };
		double rotatedResultVertexZs[N] = { 0.0 };
		double rotatedResultVertexWs[N] = { 0.0 };
		Matrix4_MultiplyVectorN<N>(
			rotationMatrixXXs, rotationMatrixXYs, rotationMatrixXZs, rotationMatrixXWs,
			rotationMatrixYXs, rotationMatrixYYs, rotationMatrixYZs, rotationMatrixYWs,
			rotationMatrixZXs, rotationMatrixZYs, rotationMatrixZZs, rotationMatrixZWs,
			rotationMatrixWXs, rotationMatrixWYs, rotationMatrixWZs, rotationMatrixWWs,
			resultVertexXs, resultVertexYs, resultVertexZs, resultVertexWs,
			rotatedResultVertexXs, rotatedResultVertexYs, rotatedResultVertexZs, rotatedResultVertexWs);

		// Apply translation matrix.
		double translatedResultVertexXs[N] = { 0.0 };
		double translatedResultVertexYs[N] = { 0.0 };
		double translatedResultVertexZs[N] = { 0.0 };
		double translatedResultVertexWs[N] = { 0.0 };
		Matrix4_MultiplyVectorN<N>(
			translationMatrixXXs, translationMatrixXYs, translationMatrixXZs, translationMatrixXWs,
			translationMatrixYXs, translationMatrixYYs, translationMatrixYZs, translationMatrixYWs,
			translationMatrixZXs, translationMatrixZYs, translationMatrixZZs, translationMatrixZWs,
			translationMatrixWXs, translationMatrixWYs, translationMatrixWZs, translationMatrixWWs,
			rotatedResultVertexXs, rotatedResultVertexYs, rotatedResultVertexZs, rotatedResultVertexWs,
			translatedResultVertexXs, translatedResultVertexYs, translatedResultVertexZs, translatedResultVertexWs);

		// Apply view-projection matrix.
		Matrix4_MultiplyVectorN<N>(
			g_viewProjMatrixXX, g_viewProjMatrixXY, g_viewProjMatrixXZ, g_viewProjMatrixXW,
			g_viewProjMatrixYX, g_viewProjMatrixYY, g_viewProjMatrixYZ, g_viewProjMatrixYW,
			g_viewProjMatrixZX, g_viewProjMatrixZY, g_viewProjMatrixZZ, g_viewProjMatrixZW,
			g_viewProjMatrixWX, g_viewProjMatrixWY, g_viewProjMatrixWZ, g_viewProjMatrixWW,
			translatedResultVertexXs, translatedResultVertexYs, translatedResultVertexZs, translatedResultVertexWs,
			outVertexXs, outVertexYs, outVertexZs, outVertexWs);
	}

	template<int N>
	void VertexShader_EntityN(const TransformCache &__restrict transformCache, const double *__restrict vertexXs, const double *__restrict vertexYs,
		const double *__restrict vertexZs, const double *__restrict vertexWs, double *__restrict outVertexXs, double *__restrict outVertexYs,
		double *__restrict outVertexZs, double *__restrict outVertexWs)
	{
		double modelViewProjMatrixXXs[N];
		double modelViewProjMatrixXYs[N];
		double modelViewProjMatrixXZs[N];
		double modelViewProjMatrixXWs[N];
		double modelViewProjMatrixYXs[N];
		double modelViewProjMatrixYYs[N];
		double modelViewProjMatrixYZs[N];
		double modelViewProjMatrixYWs[N];
		double modelViewProjMatrixZXs[N];
		double modelViewProjMatrixZYs[N];
		double modelViewProjMatrixZZs[N];
		double modelViewProjMatrixZWs[N];
		double modelViewProjMatrixWXs[N];
		double modelViewProjMatrixWYs[N];
		double modelViewProjMatrixWZs[N];
		double modelViewProjMatrixWWs[N];
		for (int i = 0; i < N; i++)
		{
			// @todo: this isn't taking meshIndex anymore
			modelViewProjMatrixXXs[i] = transformCache.modelViewProjMatrixXX;
			modelViewProjMatrixXYs[i] = transformCache.modelViewProjMatrixXY;
			modelViewProjMatrixXZs[i] = transformCache.modelViewProjMatrixXZ;
			modelViewProjMatrixXWs[i] = transformCache.modelViewProjMatrixXW;
			modelViewProjMatrixYXs[i] = transformCache.modelViewProjMatrixYX;
			modelViewProjMatrixYYs[i] = transformCache.modelViewProjMatrixYY;
			modelViewProjMatrixYZs[i] = transformCache.modelViewProjMatrixYZ;
			modelViewProjMatrixYWs[i] = transformCache.modelViewProjMatrixYW;
			modelViewProjMatrixZXs[i] = transformCache.modelViewProjMatrixZX;
			modelViewProjMatrixZYs[i] = transformCache.modelViewProjMatrixZY;
			modelViewProjMatrixZZs[i] = transformCache.modelViewProjMatrixZZ;
			modelViewProjMatrixZWs[i] = transformCache.modelViewProjMatrixZW;
			modelViewProjMatrixWXs[i] = transformCache.modelViewProjMatrixWX;
			modelViewProjMatrixWYs[i] = transformCache.modelViewProjMatrixWY;
			modelViewProjMatrixWZs[i] = transformCache.modelViewProjMatrixWZ;
			modelViewProjMatrixWWs[i] = transformCache.modelViewProjMatrixWW;
		}

		// Apply model-view-projection matrix.
		Matrix4_MultiplyVectorN<N>(
			modelViewProjMatrixXXs, modelViewProjMatrixXYs, modelViewProjMatrixXZs, modelViewProjMatrixXWs,
			modelViewProjMatrixYXs, modelViewProjMatrixYYs, modelViewProjMatrixYZs, modelViewProjMatrixYWs,
			modelViewProjMatrixZXs, modelViewProjMatrixZYs, modelViewProjMatrixZZs, modelViewProjMatrixZWs,
			modelViewProjMatrixWXs, modelViewProjMatrixWYs, modelViewProjMatrixWZs, modelViewProjMatrixWWs,
			vertexXs, vertexYs, vertexZs, vertexWs,
			outVertexXs, outVertexYs, outVertexZs, outVertexWs);
	}
}

// Pixel shaders.
namespace
{
	struct PixelShaderPerspectiveCorrection
	{
		double ndcZDepth;
		double texelPercentX;
		double texelPercentY;
	};

	struct PixelShaderTexture
	{
		const uint8_t *texels;
		int width, height;
		int widthMinusOne, heightMinusOne;
		double widthReal, heightReal;

		PixelShaderTexture()
		{
			this->texels = nullptr;
			this->width = 0;
			this->height = 0;
			this->widthMinusOne = 0;
			this->heightMinusOne = 0;
			this->widthReal = 0.0;
			this->heightReal = 0.0;
		}

		void init(const uint8_t *texels, int width, int height)
		{
			this->texels = texels;
			this->width = width;
			this->height = height;
			this->widthMinusOne = width - 1;
			this->heightMinusOne = height - 1;
			this->widthReal = static_cast<double>(width);
			this->heightReal = static_cast<double>(height);
		}
	};

	struct PixelShaderPalette
	{
		const uint32_t *colors;
		int count;

		PixelShaderPalette()
		{
			this->colors = nullptr;
			this->count = 0;
		}
	};

	struct PixelShaderLighting
	{
		const uint8_t *lightTableTexels;
		int lightLevelCount; // # of shades from light to dark.
		double lightLevelCountReal;
		int lastLightLevel;
		int texelsPerLightLevel; // Should be 256 for 8-bit colors.
		int lightLevel; // The selected row of shades between light and dark.

		PixelShaderLighting()
		{
			this->lightTableTexels = nullptr;
			this->lightLevelCount = 0;
			this->lightLevelCountReal = 0.0;
			this->lastLightLevel = -1;
			this->texelsPerLightLevel = 0;
			this->lightLevel = -1;
		}
	};

	struct PixelShaderHorizonMirror
	{
		// Based on camera forward direction as XZ vector.
		double horizonScreenSpacePointX;
		double horizonScreenSpacePointY;

		int reflectedPixelIndex;
		bool isReflectedPixelInFrameBuffer;
		uint8_t fallbackSkyColor;

		PixelShaderHorizonMirror()
		{
			this->horizonScreenSpacePointX = 0.0;
			this->horizonScreenSpacePointY = 0.0;
			this->reflectedPixelIndex = -1;
			this->isReflectedPixelInFrameBuffer = false;
			this->fallbackSkyColor = 0;
		}
	};

	struct PixelShaderUniforms
	{
		double screenSpaceAnimPercent;

		PixelShaderUniforms()
		{
			screenSpaceAnimPercent = 0.0;
		}
	};

	struct PixelShaderFrameBuffer
	{
		PixelShaderPalette palette;
		double xPercent, yPercent;
		int pixelIndex;

		PixelShaderFrameBuffer()
		{
			this->xPercent = 0.0;
			this->yPercent = 0.0;
			this->pixelIndex = -1;
		}
	};

	double g_ambientPercent;
	double g_screenSpaceAnimPercent;
	const SoftwareRenderer::ObjectTexture *g_paletteTexture; // 8-bit -> 32-bit color conversion palette.
	const SoftwareRenderer::ObjectTexture *g_lightTableTexture; // Shading/transparency look-ups.
	const SoftwareRenderer::ObjectTexture *g_skyBgTexture; // Fallback sky texture for horizon reflection shader.

	void PopulatePixelShaderGlobals(double ambientPercent, double screenSpaceAnimPercent, const SoftwareRenderer::ObjectTexture &paletteTexture,
		const SoftwareRenderer::ObjectTexture &lightTableTexture, const SoftwareRenderer::ObjectTexture &skyBgTexture)
	{
		g_ambientPercent = ambientPercent;
		g_screenSpaceAnimPercent = screenSpaceAnimPercent;
		g_paletteTexture = &paletteTexture;
		g_lightTableTexture = &lightTableTexture;
		g_skyBgTexture = &skyBgTexture;
	}

	template<bool enableDepthWrite>
	void PixelShader_Opaque(const PixelShaderPerspectiveCorrection &perspective, const PixelShaderTexture &texture,
		const PixelShaderLighting &lighting, PixelShaderFrameBuffer &frameBuffer)
	{
		const int texelX = std::clamp(static_cast<int>(perspective.texelPercentX * texture.widthReal), 0, texture.widthMinusOne);
		const int texelY = std::clamp(static_cast<int>(perspective.texelPercentY * texture.heightReal), 0, texture.heightMinusOne);
		const int texelIndex = texelX + (texelY * texture.width);
		const uint8_t texel = texture.texels[texelIndex];

		const int shadedTexelIndex = texel + (lighting.lightLevel * lighting.texelsPerLightLevel);
		const uint8_t shadedTexel = lighting.lightTableTexels[shadedTexelIndex];
		g_paletteIndexBuffer[frameBuffer.pixelIndex] = shadedTexel;

		if constexpr (enableDepthWrite)
		{
			g_depthBuffer[frameBuffer.pixelIndex] = perspective.ndcZDepth;
		}
	}

	template<bool enableDepthWrite>
	void PixelShader_OpaqueWithAlphaTestLayer(const PixelShaderPerspectiveCorrection &perspective, const PixelShaderTexture &opaqueTexture,
		const PixelShaderTexture &alphaTestTexture, const PixelShaderLighting &lighting, PixelShaderFrameBuffer &frameBuffer)
	{
		const int layerTexelX = std::clamp(static_cast<int>(perspective.texelPercentX * alphaTestTexture.widthReal), 0, alphaTestTexture.widthMinusOne);
		const int layerTexelY = std::clamp(static_cast<int>(perspective.texelPercentY * alphaTestTexture.heightReal), 0, alphaTestTexture.heightMinusOne);
		const int layerTexelIndex = layerTexelX + (layerTexelY * alphaTestTexture.width);
		uint8_t texel = alphaTestTexture.texels[layerTexelIndex];

		const bool isTransparent = texel == ArenaRenderUtils::PALETTE_INDEX_TRANSPARENT;
		if (isTransparent)
		{
			const int texelX = std::clamp(static_cast<int>(perspective.texelPercentX * opaqueTexture.widthReal), 0, opaqueTexture.widthMinusOne);
			const int texelY = std::clamp(static_cast<int>(perspective.texelPercentY * opaqueTexture.heightReal), 0, opaqueTexture.heightMinusOne);
			const int texelIndex = texelX + (texelY * opaqueTexture.width);
			texel = opaqueTexture.texels[texelIndex];
		}

		const int shadedTexelIndex = texel + (lighting.lightLevel * lighting.texelsPerLightLevel);
		const uint8_t shadedTexel = lighting.lightTableTexels[shadedTexelIndex];
		g_paletteIndexBuffer[frameBuffer.pixelIndex] = shadedTexel;

		if constexpr (enableDepthWrite)
		{
			g_depthBuffer[frameBuffer.pixelIndex] = perspective.ndcZDepth;
		}
	}

	constexpr int SCREEN_SPACE_ANIM_HEIGHT = 100; // @todo dehardcode w/ another parameter
	template<bool enableDepthWrite>
	void PixelShader_OpaqueScreenSpaceAnimation(const PixelShaderPerspectiveCorrection &perspective, const PixelShaderTexture &texture, const PixelShaderLighting &lighting, const PixelShaderUniforms &uniforms, PixelShaderFrameBuffer &frameBuffer)
	{
		// @todo chasms: determine how many pixels the original texture should cover, based on what percentage the original texture height is over the original screen height.		
		const int texelX = std::clamp(static_cast<int>(frameBuffer.xPercent * texture.widthReal), 0, texture.widthMinusOne);

		constexpr int frameHeight = SCREEN_SPACE_ANIM_HEIGHT;
		const int frameCount = texture.height / frameHeight;
		const double animPercent = uniforms.screenSpaceAnimPercent;
		const int currentFrameIndex = std::clamp(static_cast<int>(static_cast<double>(frameCount) * animPercent), 0, frameCount - 1);

		const double frameBufferV = frameBuffer.yPercent * 2.0;
		const double normalizedV = frameBufferV >= 1.0 ? (frameBufferV - 1.0) : frameBufferV;
		const double sampleV = (normalizedV / static_cast<double>(frameCount)) + (static_cast<double>(currentFrameIndex) / static_cast<double>(frameCount));
		const int texelY = std::clamp(static_cast<int>(sampleV * texture.heightReal), 0, texture.heightMinusOne);

		const int texelIndex = texelX + (texelY * texture.width);
		const uint8_t texel = texture.texels[texelIndex];

		const int shadedTexelIndex = texel + (lighting.lightLevel * lighting.texelsPerLightLevel);
		const uint8_t shadedTexel = lighting.lightTableTexels[shadedTexelIndex];
		g_paletteIndexBuffer[frameBuffer.pixelIndex] = shadedTexel;

		if constexpr (enableDepthWrite)
		{
			g_depthBuffer[frameBuffer.pixelIndex] = perspective.ndcZDepth;
		}
	}

	template<bool enableDepthWrite>
	void PixelShader_OpaqueScreenSpaceAnimationWithAlphaTestLayer(const PixelShaderPerspectiveCorrection &perspective, const PixelShaderTexture &opaqueTexture,
		const PixelShaderTexture &alphaTestTexture, const PixelShaderLighting &lighting, const PixelShaderUniforms &uniforms, PixelShaderFrameBuffer &frameBuffer)
	{
		const int layerTexelX = std::clamp(static_cast<int>(perspective.texelPercentX * alphaTestTexture.widthReal), 0, alphaTestTexture.widthMinusOne);
		const int layerTexelY = std::clamp(static_cast<int>(perspective.texelPercentY * alphaTestTexture.heightReal), 0, alphaTestTexture.heightMinusOne);
		const int layerTexelIndex = layerTexelX + (layerTexelY * alphaTestTexture.width);
		uint8_t texel = alphaTestTexture.texels[layerTexelIndex];

		const bool isTransparent = texel == ArenaRenderUtils::PALETTE_INDEX_TRANSPARENT;
		if (isTransparent)
		{
			const int texelX = std::clamp(static_cast<int>(frameBuffer.xPercent * opaqueTexture.widthReal), 0, opaqueTexture.widthMinusOne);

			constexpr int frameHeight = SCREEN_SPACE_ANIM_HEIGHT;
			const int frameCount = opaqueTexture.height / frameHeight;
			const double animPercent = uniforms.screenSpaceAnimPercent;
			const int currentFrameIndex = std::clamp(static_cast<int>(static_cast<double>(frameCount) * animPercent), 0, frameCount - 1);

			const double frameBufferV = frameBuffer.yPercent * 2.0;
			const double normalizedV = frameBufferV >= 1.0 ? (frameBufferV - 1.0) : frameBufferV;
			const double sampleV = (normalizedV / static_cast<double>(frameCount)) + (static_cast<double>(currentFrameIndex) / static_cast<double>(frameCount));
			const int texelY = std::clamp(static_cast<int>(sampleV * opaqueTexture.heightReal), 0, opaqueTexture.heightMinusOne);

			const int texelIndex = texelX + (texelY * opaqueTexture.width);
			texel = opaqueTexture.texels[texelIndex];
		}

		const int shadedTexelIndex = texel + (lighting.lightLevel * lighting.texelsPerLightLevel);
		const uint8_t shadedTexel = lighting.lightTableTexels[shadedTexelIndex];
		g_paletteIndexBuffer[frameBuffer.pixelIndex] = shadedTexel;

		if constexpr (enableDepthWrite)
		{
			g_depthBuffer[frameBuffer.pixelIndex] = perspective.ndcZDepth;
		}
	}

	template<bool enableDepthWrite>
	void PixelShader_AlphaTested(const PixelShaderPerspectiveCorrection &perspective, const PixelShaderTexture &texture,
		const PixelShaderLighting &lighting, PixelShaderFrameBuffer &frameBuffer)
	{
		const int texelX = std::clamp(static_cast<int>(perspective.texelPercentX * texture.widthReal), 0, texture.widthMinusOne);
		const int texelY = std::clamp(static_cast<int>(perspective.texelPercentY * texture.heightReal), 0, texture.heightMinusOne);
		const int texelIndex = texelX + (texelY * texture.width);
		const uint8_t texel = texture.texels[texelIndex];

		const bool isTransparent = texel == ArenaRenderUtils::PALETTE_INDEX_TRANSPARENT;
		if (isTransparent)
		{
			return;
		}

		const int shadedTexelIndex = texel + (lighting.lightLevel * lighting.texelsPerLightLevel);
		const uint8_t shadedTexel = lighting.lightTableTexels[shadedTexelIndex];
		g_paletteIndexBuffer[frameBuffer.pixelIndex] = shadedTexel;

		if constexpr (enableDepthWrite)
		{
			g_depthBuffer[frameBuffer.pixelIndex] = perspective.ndcZDepth;
		}
	}

	template<bool enableDepthWrite>
	void PixelShader_AlphaTestedWithVariableTexCoordUMin(const PixelShaderPerspectiveCorrection &perspective, const PixelShaderTexture &texture,
		double uMin, const PixelShaderLighting &lighting, PixelShaderFrameBuffer &frameBuffer)
	{
		const double u = std::clamp(uMin + ((1.0 - uMin) * perspective.texelPercentX), uMin, 1.0);
		const int texelX = std::clamp(static_cast<int>(u * texture.widthReal), 0, texture.widthMinusOne);
		const int texelY = std::clamp(static_cast<int>(perspective.texelPercentY * texture.height), 0, texture.heightMinusOne);
		const int texelIndex = texelX + (texelY * texture.width);
		const uint8_t texel = texture.texels[texelIndex];

		const bool isTransparent = texel == ArenaRenderUtils::PALETTE_INDEX_TRANSPARENT;
		if (isTransparent)
		{
			return;
		}

		const int shadedTexelIndex = texel + (lighting.lightLevel * lighting.texelsPerLightLevel);
		const uint8_t shadedTexel = lighting.lightTableTexels[shadedTexelIndex];
		g_paletteIndexBuffer[frameBuffer.pixelIndex] = shadedTexel;

		if constexpr (enableDepthWrite)
		{
			g_depthBuffer[frameBuffer.pixelIndex] = perspective.ndcZDepth;
		}
	}

	template<bool enableDepthWrite>
	void PixelShader_AlphaTestedWithVariableTexCoordVMin(const PixelShaderPerspectiveCorrection &perspective, const PixelShaderTexture &texture,
		double vMin, const PixelShaderLighting &lighting, PixelShaderFrameBuffer &frameBuffer)
	{
		const int texelX = std::clamp(static_cast<int>(perspective.texelPercentX * texture.widthReal), 0, texture.widthMinusOne);
		const double v = std::clamp(vMin + ((1.0 - vMin) * perspective.texelPercentY), vMin, 1.0);
		const int texelY = std::clamp(static_cast<int>(v * texture.heightReal), 0, texture.heightMinusOne);

		const int texelIndex = texelX + (texelY * texture.width);
		const uint8_t texel = texture.texels[texelIndex];

		const bool isTransparent = texel == ArenaRenderUtils::PALETTE_INDEX_TRANSPARENT;
		if (isTransparent)
		{
			return;
		}

		const int shadedTexelIndex = texel + (lighting.lightLevel * lighting.texelsPerLightLevel);
		const uint8_t shadedTexel = lighting.lightTableTexels[shadedTexelIndex];
		g_paletteIndexBuffer[frameBuffer.pixelIndex] = shadedTexel;

		if constexpr (enableDepthWrite)
		{
			g_depthBuffer[frameBuffer.pixelIndex] = perspective.ndcZDepth;
		}
	}

	template<bool enableDepthWrite>
	void PixelShader_AlphaTestedWithPaletteIndexLookup(const PixelShaderPerspectiveCorrection &perspective, const PixelShaderTexture &texture,
		const PixelShaderTexture &lookupTexture, const PixelShaderLighting &lighting, PixelShaderFrameBuffer &frameBuffer)
	{
		const int texelX = std::clamp(static_cast<int>(perspective.texelPercentX * texture.widthReal), 0, texture.widthMinusOne);
		const int texelY = std::clamp(static_cast<int>(perspective.texelPercentY * texture.heightReal), 0, texture.heightMinusOne);
		const int texelIndex = texelX + (texelY * texture.width);
		const uint8_t texel = texture.texels[texelIndex];

		const bool isTransparent = texel == ArenaRenderUtils::PALETTE_INDEX_TRANSPARENT;
		if (isTransparent)
		{
			return;
		}

		const uint8_t replacementTexel = lookupTexture.texels[texel];

		const int shadedTexelIndex = replacementTexel + (lighting.lightLevel * lighting.texelsPerLightLevel);
		const uint8_t shadedTexel = lighting.lightTableTexels[shadedTexelIndex];
		g_paletteIndexBuffer[frameBuffer.pixelIndex] = shadedTexel;

		if constexpr (enableDepthWrite)
		{
			g_depthBuffer[frameBuffer.pixelIndex] = perspective.ndcZDepth;
		}
	}

	template<bool enableDepthWrite>
	void PixelShader_AlphaTestedWithLightLevelColor(const PixelShaderPerspectiveCorrection &perspective, const PixelShaderTexture &texture,
		const PixelShaderLighting &lighting, PixelShaderFrameBuffer &frameBuffer)
	{
		const int texelX = std::clamp(static_cast<int>(perspective.texelPercentX * texture.widthReal), 0, texture.widthMinusOne);
		const int texelY = std::clamp(static_cast<int>(perspective.texelPercentY * texture.heightReal), 0, texture.heightMinusOne);
		const int texelIndex = texelX + (texelY * texture.width);
		const uint8_t texel = texture.texels[texelIndex];

		const bool isTransparent = texel == ArenaRenderUtils::PALETTE_INDEX_TRANSPARENT;
		if (isTransparent)
		{
			return;
		}

		const int lightTableTexelIndex = texel + (lighting.lightLevel * lighting.texelsPerLightLevel);
		const uint8_t resultTexel = lighting.lightTableTexels[lightTableTexelIndex];

		g_paletteIndexBuffer[frameBuffer.pixelIndex] = resultTexel;

		if constexpr (enableDepthWrite)
		{
			g_depthBuffer[frameBuffer.pixelIndex] = perspective.ndcZDepth;
		}
	}

	template<bool enableDepthWrite>
	void PixelShader_AlphaTestedWithLightLevelOpacity(const PixelShaderPerspectiveCorrection &perspective, const PixelShaderTexture &texture,
		const PixelShaderLighting &lighting, PixelShaderFrameBuffer &frameBuffer)
	{
		const int texelX = std::clamp(static_cast<int>(perspective.texelPercentX * texture.widthReal), 0, texture.widthMinusOne);
		const int texelY = std::clamp(static_cast<int>(perspective.texelPercentY * texture.heightReal), 0, texture.heightMinusOne);
		const int texelIndex = texelX + (texelY * texture.width);
		const uint8_t texel = texture.texels[texelIndex];

		const bool isTransparent = texel == ArenaRenderUtils::PALETTE_INDEX_TRANSPARENT;
		if (isTransparent)
		{
			return;
		}

		int lightTableTexelIndex;
		if (ArenaRenderUtils::isLightLevelTexel(texel))
		{
			const int lightLevel = static_cast<int>(texel) - ArenaRenderUtils::PALETTE_INDEX_LIGHT_LEVEL_LOWEST;
			const uint8_t prevFrameBufferPixel = g_paletteIndexBuffer[frameBuffer.pixelIndex];
			lightTableTexelIndex = prevFrameBufferPixel + (lightLevel * lighting.texelsPerLightLevel);
		}
		else
		{
			const int lightTableOffset = lighting.lightLevel * lighting.texelsPerLightLevel;
			if (texel == ArenaRenderUtils::PALETTE_INDEX_LIGHT_LEVEL_SRC1)
			{
				lightTableTexelIndex = lightTableOffset + ArenaRenderUtils::PALETTE_INDEX_LIGHT_LEVEL_DST1;
			}
			else if (texel == ArenaRenderUtils::PALETTE_INDEX_LIGHT_LEVEL_SRC2)
			{
				lightTableTexelIndex = lightTableOffset + ArenaRenderUtils::PALETTE_INDEX_LIGHT_LEVEL_DST2;
			}
			else
			{
				lightTableTexelIndex = lightTableOffset + texel;
			}
		}

		const uint8_t resultTexel = lighting.lightTableTexels[lightTableTexelIndex];
		g_paletteIndexBuffer[frameBuffer.pixelIndex] = resultTexel;

		if constexpr (enableDepthWrite)
		{
			g_depthBuffer[frameBuffer.pixelIndex] = perspective.ndcZDepth;
		}
	}

	template<bool enableDepthWrite>
	void PixelShader_AlphaTestedWithPreviousBrightnessLimit(const PixelShaderPerspectiveCorrection &perspective,
		const PixelShaderTexture &texture, PixelShaderFrameBuffer &frameBuffer)
	{
		constexpr int brightnessLimit = 0x3F; // Highest value each RGB component can be.
		constexpr uint8_t brightnessMask = ~brightnessLimit;
		constexpr uint32_t brightnessMaskR = brightnessMask << 16;
		constexpr uint32_t brightnessMaskG = brightnessMask << 8;
		constexpr uint32_t brightnessMaskB = brightnessMask;
		constexpr uint32_t brightnessMaskRGB = brightnessMaskR | brightnessMaskG | brightnessMaskB;

		const uint8_t prevFrameBufferPixel = g_paletteIndexBuffer[frameBuffer.pixelIndex];
		const uint32_t prevFrameBufferColor = frameBuffer.palette.colors[prevFrameBufferPixel];
		const bool isDarkEnough = (prevFrameBufferColor & brightnessMaskRGB) == 0;
		if (!isDarkEnough)
		{
			return;
		}

		const int texelX = std::clamp(static_cast<int>(perspective.texelPercentX * texture.widthReal), 0, texture.widthMinusOne);
		const int texelY = std::clamp(static_cast<int>(perspective.texelPercentY * texture.heightReal), 0, texture.heightMinusOne);
		const int texelIndex = texelX + (texelY * texture.width);
		const uint8_t texel = texture.texels[texelIndex];

		const bool isTransparent = texel == ArenaRenderUtils::PALETTE_INDEX_TRANSPARENT;
		if (isTransparent)
		{
			return;
		}

		g_paletteIndexBuffer[frameBuffer.pixelIndex] = texel;

		if constexpr (enableDepthWrite)
		{
			g_depthBuffer[frameBuffer.pixelIndex] = perspective.ndcZDepth;
		}
	}

	template<bool enableDepthWrite>
	void PixelShader_AlphaTestedWithHorizonMirror(const PixelShaderPerspectiveCorrection &perspective,
		const PixelShaderTexture &texture, const PixelShaderHorizonMirror &horizon, const PixelShaderLighting &lighting,
		PixelShaderFrameBuffer &frameBuffer)
	{
		const int texelX = std::clamp(static_cast<int>(perspective.texelPercentX * texture.widthReal), 0, texture.widthMinusOne);
		const int texelY = std::clamp(static_cast<int>(perspective.texelPercentY * texture.heightReal), 0, texture.heightMinusOne);
		const int texelIndex = texelX + (texelY * texture.width);
		const uint8_t texel = texture.texels[texelIndex];

		const bool isTransparent = texel == ArenaRenderUtils::PALETTE_INDEX_TRANSPARENT;
		if (isTransparent)
		{
			return;
		}

		uint8_t resultTexel;
		const bool isReflective = texel == ArenaRenderUtils::PALETTE_INDEX_PUDDLE_EVEN_ROW;
		if (isReflective)
		{
			if (horizon.isReflectedPixelInFrameBuffer)
			{
				const uint8_t mirroredTexel = g_paletteIndexBuffer[horizon.reflectedPixelIndex];
				resultTexel = mirroredTexel;
			}
			else
			{
				resultTexel = horizon.fallbackSkyColor;
			}
		}
		else
		{
			const int shadedTexelIndex = texel + (lighting.lightLevel * lighting.texelsPerLightLevel);
			resultTexel = lighting.lightTableTexels[shadedTexelIndex];
		}

		g_paletteIndexBuffer[frameBuffer.pixelIndex] = resultTexel;

		if constexpr (enableDepthWrite)
		{
			g_depthBuffer[frameBuffer.pixelIndex] = perspective.ndcZDepth;
		}
	}
}

// Mesh processing, vertex shader execution.
namespace
{
	constexpr int MAX_DRAW_CALL_MESH_TRIANGLES = 1024; // The most triangles a draw call mesh can have. Used with vertex shading.
	constexpr int MAX_VERTEX_SHADING_CACHE_TRIANGLES = MAX_DRAW_CALL_MESH_TRIANGLES * 2; // The most unshaded triangles that can be cached for the vertex shader loop.
	constexpr int MAX_CLIPPED_MESH_TRIANGLES = 4096; // The most triangles a processed clip space mesh can have when passed to the rasterizer.
	constexpr int MAX_CLIPPED_TRIANGLE_TRIANGLES = 64; // The most triangles a triangle can generate after being clipped by all clip planes.

	// One per group of mesh process caches, for improving number crunching efficiency with vertex shading by
	// keeping the triangle count much higher than the average 2 per draw call.
	struct VertexShaderInputCache
	{
		double unshadedV0Xs[MAX_VERTEX_SHADING_CACHE_TRIANGLES];
		double unshadedV0Ys[MAX_VERTEX_SHADING_CACHE_TRIANGLES];
		double unshadedV0Zs[MAX_VERTEX_SHADING_CACHE_TRIANGLES];
		double unshadedV0Ws[MAX_VERTEX_SHADING_CACHE_TRIANGLES];
		double unshadedV1Xs[MAX_VERTEX_SHADING_CACHE_TRIANGLES];
		double unshadedV1Ys[MAX_VERTEX_SHADING_CACHE_TRIANGLES];
		double unshadedV1Zs[MAX_VERTEX_SHADING_CACHE_TRIANGLES];
		double unshadedV1Ws[MAX_VERTEX_SHADING_CACHE_TRIANGLES];
		double unshadedV2Xs[MAX_VERTEX_SHADING_CACHE_TRIANGLES];
		double unshadedV2Ys[MAX_VERTEX_SHADING_CACHE_TRIANGLES];
		double unshadedV2Zs[MAX_VERTEX_SHADING_CACHE_TRIANGLES];
		double unshadedV2Ws[MAX_VERTEX_SHADING_CACHE_TRIANGLES];
		double uv0Xs[MAX_VERTEX_SHADING_CACHE_TRIANGLES];
		double uv0Ys[MAX_VERTEX_SHADING_CACHE_TRIANGLES];
		double uv1Xs[MAX_VERTEX_SHADING_CACHE_TRIANGLES];
		double uv1Ys[MAX_VERTEX_SHADING_CACHE_TRIANGLES];
		double uv2Xs[MAX_VERTEX_SHADING_CACHE_TRIANGLES];
		double uv2Ys[MAX_VERTEX_SHADING_CACHE_TRIANGLES];
		int triangleCount;
	};

	// Vertex shader results to be iterated over during clipping.
	struct VertexShaderOutputCache
	{
		double shadedV0XYZWArray[MAX_DRAW_CALL_MESH_TRIANGLES][4];
		double shadedV1XYZWArray[MAX_DRAW_CALL_MESH_TRIANGLES][4];
		double shadedV2XYZWArray[MAX_DRAW_CALL_MESH_TRIANGLES][4];
		double uv0XYArray[MAX_DRAW_CALL_MESH_TRIANGLES][2];
		double uv1XYArray[MAX_DRAW_CALL_MESH_TRIANGLES][2];
		double uv2XYArray[MAX_DRAW_CALL_MESH_TRIANGLES][2];
		int triangleWriteCount; // This should match the draw call triangle count.
	};

	struct ClippingOutputCache
	{
		// Triangles generated by clipping the current mesh. These are sent to the rasterizer.
		double clipSpaceMeshV0XYZWArray[MAX_CLIPPED_MESH_TRIANGLES][4];
		double clipSpaceMeshV1XYZWArray[MAX_CLIPPED_MESH_TRIANGLES][4];
		double clipSpaceMeshV2XYZWArray[MAX_CLIPPED_MESH_TRIANGLES][4];
		double clipSpaceMeshUV0XYArray[MAX_CLIPPED_MESH_TRIANGLES][2];
		double clipSpaceMeshUV1XYArray[MAX_CLIPPED_MESH_TRIANGLES][2];
		double clipSpaceMeshUV2XYArray[MAX_CLIPPED_MESH_TRIANGLES][2];
		int clipSpaceMeshTriangleCount; // Number of triangles in these clip space meshes to be rasterized.

		// Triangles generated by clipping the current triangle against clipping planes.
		double clipSpaceTriangleV0XYZWArray[MAX_CLIPPED_TRIANGLE_TRIANGLES][4];
		double clipSpaceTriangleV1XYZWArray[MAX_CLIPPED_TRIANGLE_TRIANGLES][4];
		double clipSpaceTriangleV2XYZWArray[MAX_CLIPPED_TRIANGLE_TRIANGLES][4];
		double clipSpaceTriangleUV0XYArray[MAX_CLIPPED_TRIANGLE_TRIANGLES][2];
		double clipSpaceTriangleUV1XYArray[MAX_CLIPPED_TRIANGLE_TRIANGLES][2];
		double clipSpaceTriangleUV2XYArray[MAX_CLIPPED_TRIANGLE_TRIANGLES][2];
	};

	std::atomic<int> g_totalPresentedTriangleCount = 0; // Triangles the rasterizer spends any time attempting to shade pixels for.

	void ClearTriangleTotalCounts()
	{
		g_totalPresentedTriangleCount = 0;
	}

	// Handles the vertex/attribute/index buffer lookups for more efficient processing later.
	void ProcessMeshBufferLookups(const DrawCallCache &drawCallCache, VertexShaderInputCache &vertexShaderInputCache)
	{
		vertexShaderInputCache.triangleCount = 0;

		// Append vertices and texture coordinates into big arrays. The incoming meshes are likely tiny like 2 triangles each,
		// so this makes the total triangle loop longer for ease of number crunching.
		const double *verticesPtr = drawCallCache.vertexBuffer->vertices.begin();
		const double *texCoordsPtr = drawCallCache.texCoordBuffer->attributes.begin();
		const SoftwareRenderer::IndexBuffer &indexBuffer = *drawCallCache.indexBuffer;
		const int32_t *indicesPtr = indexBuffer.indices.begin();
		const int meshTriangleCount = indexBuffer.triangleCount;
		DebugAssert(meshTriangleCount <= MAX_DRAW_CALL_MESH_TRIANGLES);

		int writeIndex = 0;
		DebugAssert((writeIndex + meshTriangleCount) <= MAX_VERTEX_SHADING_CACHE_TRIANGLES);
		for (int triangleIndex = 0; triangleIndex < meshTriangleCount; triangleIndex++)
		{
			constexpr int indicesPerTriangle = 3;
			constexpr int positionComponentsPerVertex = 3;
			constexpr int texCoordComponentsPerVertex = 2;
			const int indexBufferBase = triangleIndex * indicesPerTriangle;
			const int32_t index0 = indicesPtr[indexBufferBase];
			const int32_t index1 = indicesPtr[indexBufferBase + 1];
			const int32_t index2 = indicesPtr[indexBufferBase + 2];
			const int32_t v0Index = index0 * positionComponentsPerVertex;
			const int32_t v1Index = index1 * positionComponentsPerVertex;
			const int32_t v2Index = index2 * positionComponentsPerVertex;
			const int32_t uv0Index = index0 * texCoordComponentsPerVertex;
			const int32_t uv1Index = index1 * texCoordComponentsPerVertex;
			const int32_t uv2Index = index2 * texCoordComponentsPerVertex;
			vertexShaderInputCache.unshadedV0Xs[writeIndex] = verticesPtr[v0Index];
			vertexShaderInputCache.unshadedV0Ys[writeIndex] = verticesPtr[v0Index + 1];
			vertexShaderInputCache.unshadedV0Zs[writeIndex] = verticesPtr[v0Index + 2];
			vertexShaderInputCache.unshadedV0Ws[writeIndex] = 1.0;
			vertexShaderInputCache.unshadedV1Xs[writeIndex] = verticesPtr[v1Index];
			vertexShaderInputCache.unshadedV1Ys[writeIndex] = verticesPtr[v1Index + 1];
			vertexShaderInputCache.unshadedV1Zs[writeIndex] = verticesPtr[v1Index + 2];
			vertexShaderInputCache.unshadedV1Ws[writeIndex] = 1.0;
			vertexShaderInputCache.unshadedV2Xs[writeIndex] = verticesPtr[v2Index];
			vertexShaderInputCache.unshadedV2Ys[writeIndex] = verticesPtr[v2Index + 1];
			vertexShaderInputCache.unshadedV2Zs[writeIndex] = verticesPtr[v2Index + 2];
			vertexShaderInputCache.unshadedV2Ws[writeIndex] = 1.0;
			vertexShaderInputCache.uv0Xs[writeIndex] = texCoordsPtr[uv0Index];
			vertexShaderInputCache.uv0Ys[writeIndex] = texCoordsPtr[uv0Index + 1];
			vertexShaderInputCache.uv1Xs[writeIndex] = texCoordsPtr[uv1Index];
			vertexShaderInputCache.uv1Ys[writeIndex] = texCoordsPtr[uv1Index + 1];
			vertexShaderInputCache.uv2Xs[writeIndex] = texCoordsPtr[uv2Index];
			vertexShaderInputCache.uv2Ys[writeIndex] = texCoordsPtr[uv2Index + 1];
			writeIndex++;
		}

		vertexShaderInputCache.triangleCount = meshTriangleCount;
	}

	void CalculateVertexShaderTransforms(TransformCache &transformCache)
	{
		double rotationScaleMatrixXX, rotationScaleMatrixXY, rotationScaleMatrixXZ, rotationScaleMatrixXW;
		double rotationScaleMatrixYX, rotationScaleMatrixYY, rotationScaleMatrixYZ, rotationScaleMatrixYW;
		double rotationScaleMatrixZX, rotationScaleMatrixZY, rotationScaleMatrixZZ, rotationScaleMatrixZW;
		double rotationScaleMatrixWX, rotationScaleMatrixWY, rotationScaleMatrixWZ, rotationScaleMatrixWW;
		double modelMatrixXX, modelMatrixXY, modelMatrixXZ, modelMatrixXW;
		double modelMatrixYX, modelMatrixYY, modelMatrixYZ, modelMatrixYW;
		double modelMatrixZX, modelMatrixZY, modelMatrixZZ, modelMatrixZW;
		double modelMatrixWX, modelMatrixWY, modelMatrixWZ, modelMatrixWW;

		// Rotation-scale matrix
		Matrix4_MultiplyMatrixN<1>(
			&transformCache.rotationMatrixXX, &transformCache.rotationMatrixXY, &transformCache.rotationMatrixXZ, &transformCache.rotationMatrixXW,
			&transformCache.rotationMatrixYX, &transformCache.rotationMatrixYY, &transformCache.rotationMatrixYZ, &transformCache.rotationMatrixYW,
			&transformCache.rotationMatrixZX, &transformCache.rotationMatrixZY, &transformCache.rotationMatrixZZ, &transformCache.rotationMatrixZW,
			&transformCache.rotationMatrixWX, &transformCache.rotationMatrixWY, &transformCache.rotationMatrixWZ, &transformCache.rotationMatrixWW,
			&transformCache.scaleMatrixXX, &transformCache.scaleMatrixXY, &transformCache.scaleMatrixXZ, &transformCache.scaleMatrixXW,
			&transformCache.scaleMatrixYX, &transformCache.scaleMatrixYY, &transformCache.scaleMatrixYZ, &transformCache.scaleMatrixYW,
			&transformCache.scaleMatrixZX, &transformCache.scaleMatrixZY, &transformCache.scaleMatrixZZ, &transformCache.scaleMatrixZW,
			&transformCache.scaleMatrixWX, &transformCache.scaleMatrixWY, &transformCache.scaleMatrixWZ, &transformCache.scaleMatrixWW,
			&rotationScaleMatrixXX, &rotationScaleMatrixXY, &rotationScaleMatrixXZ, &rotationScaleMatrixXW,
			&rotationScaleMatrixYX, &rotationScaleMatrixYY, &rotationScaleMatrixYZ, &rotationScaleMatrixYW,
			&rotationScaleMatrixZX, &rotationScaleMatrixZY, &rotationScaleMatrixZZ, &rotationScaleMatrixZW,
			&rotationScaleMatrixWX, &rotationScaleMatrixWY, &rotationScaleMatrixWZ, &rotationScaleMatrixWW);

		// Model matrix
		Matrix4_MultiplyMatrixN<1>(
			&transformCache.translationMatrixXX, &transformCache.translationMatrixXY, &transformCache.translationMatrixXZ, &transformCache.translationMatrixXW,
			&transformCache.translationMatrixYX, &transformCache.translationMatrixYY, &transformCache.translationMatrixYZ, &transformCache.translationMatrixYW,
			&transformCache.translationMatrixZX, &transformCache.translationMatrixZY, &transformCache.translationMatrixZZ, &transformCache.translationMatrixZW,
			&transformCache.translationMatrixWX, &transformCache.translationMatrixWY, &transformCache.translationMatrixWZ, &transformCache.translationMatrixWW,
			&rotationScaleMatrixXX, &rotationScaleMatrixXY, &rotationScaleMatrixXZ, &rotationScaleMatrixXW,
			&rotationScaleMatrixYX, &rotationScaleMatrixYY, &rotationScaleMatrixYZ, &rotationScaleMatrixYW,
			&rotationScaleMatrixZX, &rotationScaleMatrixZY, &rotationScaleMatrixZZ, &rotationScaleMatrixZW,
			&rotationScaleMatrixWX, &rotationScaleMatrixWY, &rotationScaleMatrixWZ, &rotationScaleMatrixWW,
			&modelMatrixXX, &modelMatrixXY, &modelMatrixXZ, &modelMatrixXW,
			&modelMatrixYX, &modelMatrixYY, &modelMatrixYZ, &modelMatrixYW,
			&modelMatrixZX, &modelMatrixZY, &modelMatrixZZ, &modelMatrixZW,
			&modelMatrixWX, &modelMatrixWY, &modelMatrixWZ, &modelMatrixWW);

		// Model-view-projection matrix
		Matrix4_MultiplyMatrixN<1>(
			g_viewProjMatrixXX, g_viewProjMatrixXY, g_viewProjMatrixXZ, g_viewProjMatrixXW,
			g_viewProjMatrixYX, g_viewProjMatrixYY, g_viewProjMatrixYZ, g_viewProjMatrixYW,
			g_viewProjMatrixZX, g_viewProjMatrixZY, g_viewProjMatrixZZ, g_viewProjMatrixZW,
			g_viewProjMatrixWX, g_viewProjMatrixWY, g_viewProjMatrixWZ, g_viewProjMatrixWW,
			&modelMatrixXX, &modelMatrixXY, &modelMatrixXZ, &modelMatrixXW,
			&modelMatrixYX, &modelMatrixYY, &modelMatrixYZ, &modelMatrixYW,
			&modelMatrixZX, &modelMatrixZY, &modelMatrixZZ, &modelMatrixZW,
			&modelMatrixWX, &modelMatrixWY, &modelMatrixWZ, &modelMatrixWW,
			&transformCache.modelViewProjMatrixXX, &transformCache.modelViewProjMatrixXY, &transformCache.modelViewProjMatrixXZ, &transformCache.modelViewProjMatrixXW,
			&transformCache.modelViewProjMatrixYX, &transformCache.modelViewProjMatrixYY, &transformCache.modelViewProjMatrixYZ, &transformCache.modelViewProjMatrixYW,
			&transformCache.modelViewProjMatrixZX, &transformCache.modelViewProjMatrixZY, &transformCache.modelViewProjMatrixZZ, &transformCache.modelViewProjMatrixZW,
			&transformCache.modelViewProjMatrixWX, &transformCache.modelViewProjMatrixWY, &transformCache.modelViewProjMatrixWZ, &transformCache.modelViewProjMatrixWW);
	}

	// Converts the mesh's world space vertices to clip space.
	template<VertexShaderType vertexShaderType>
	void ProcessVertexShadersInternal(const TransformCache &transformCache, const VertexShaderInputCache &vertexShaderInputCache,
		VertexShaderOutputCache &vertexShaderOutputCache)
	{
		vertexShaderOutputCache.triangleWriteCount = 0;

		// Run vertex shaders on each triangle and store the results for clipping.
		const int triangleCount = vertexShaderInputCache.triangleCount;
		int triangleIndex = 0;
		while (triangleIndex < triangleCount)
		{
			const double unshadedV0Xs[1] = { vertexShaderInputCache.unshadedV0Xs[triangleIndex] };
			const double unshadedV0Ys[1] = { vertexShaderInputCache.unshadedV0Ys[triangleIndex] };
			const double unshadedV0Zs[1] = { vertexShaderInputCache.unshadedV0Zs[triangleIndex] };
			const double unshadedV0Ws[1] = { vertexShaderInputCache.unshadedV0Ws[triangleIndex] };
			const double unshadedV1Xs[1] = { vertexShaderInputCache.unshadedV1Xs[triangleIndex] };
			const double unshadedV1Ys[1] = { vertexShaderInputCache.unshadedV1Ys[triangleIndex] };
			const double unshadedV1Zs[1] = { vertexShaderInputCache.unshadedV1Zs[triangleIndex] };
			const double unshadedV1Ws[1] = { vertexShaderInputCache.unshadedV1Ws[triangleIndex] };
			const double unshadedV2Xs[1] = { vertexShaderInputCache.unshadedV2Xs[triangleIndex] };
			const double unshadedV2Ys[1] = { vertexShaderInputCache.unshadedV2Ys[triangleIndex] };
			const double unshadedV2Zs[1] = { vertexShaderInputCache.unshadedV2Zs[triangleIndex] };
			const double unshadedV2Ws[1] = { vertexShaderInputCache.unshadedV2Ws[triangleIndex] };
			double shadedV0Xs[1] = { 0.0 };
			double shadedV0Ys[1] = { 0.0 };
			double shadedV0Zs[1] = { 0.0 };
			double shadedV0Ws[1] = { 0.0 };
			double shadedV1Xs[1] = { 0.0 };
			double shadedV1Ys[1] = { 0.0 };
			double shadedV1Zs[1] = { 0.0 };
			double shadedV1Ws[1] = { 0.0 };
			double shadedV2Xs[1] = { 0.0 };
			double shadedV2Ys[1] = { 0.0 };
			double shadedV2Zs[1] = { 0.0 };
			double shadedV2Ws[1] = { 0.0 };

			if constexpr (vertexShaderType == VertexShaderType::Basic)
			{
				VertexShader_BasicN<1>(transformCache, unshadedV0Xs, unshadedV0Ys, unshadedV0Zs, unshadedV0Ws, shadedV0Xs, shadedV0Ys, shadedV0Zs, shadedV0Ws);
				VertexShader_BasicN<1>(transformCache, unshadedV1Xs, unshadedV1Ys, unshadedV1Zs, unshadedV1Ws, shadedV1Xs, shadedV1Ys, shadedV1Zs, shadedV1Ws);
				VertexShader_BasicN<1>(transformCache, unshadedV2Xs, unshadedV2Ys, unshadedV2Zs, unshadedV2Ws, shadedV2Xs, shadedV2Ys, shadedV2Zs, shadedV2Ws);
			}
			else if (vertexShaderType == VertexShaderType::RaisingDoor)
			{
				VertexShader_RaisingDoorN<1>(transformCache, unshadedV0Xs, unshadedV0Ys, unshadedV0Zs, unshadedV0Ws, shadedV0Xs, shadedV0Ys, shadedV0Zs, shadedV0Ws);
				VertexShader_RaisingDoorN<1>(transformCache, unshadedV1Xs, unshadedV1Ys, unshadedV1Zs, unshadedV1Ws, shadedV1Xs, shadedV1Ys, shadedV1Zs, shadedV1Ws);
				VertexShader_RaisingDoorN<1>(transformCache, unshadedV2Xs, unshadedV2Ys, unshadedV2Zs, unshadedV2Ws, shadedV2Xs, shadedV2Ys, shadedV2Zs, shadedV2Ws);
			}
			else if (vertexShaderType == VertexShaderType::Entity)
			{
				VertexShader_EntityN<1>(transformCache, unshadedV0Xs, unshadedV0Ys, unshadedV0Zs, unshadedV0Ws, shadedV0Xs, shadedV0Ys, shadedV0Zs, shadedV0Ws);
				VertexShader_EntityN<1>(transformCache, unshadedV1Xs, unshadedV1Ys, unshadedV1Zs, unshadedV1Ws, shadedV1Xs, shadedV1Ys, shadedV1Zs, shadedV1Ws);
				VertexShader_EntityN<1>(transformCache, unshadedV2Xs, unshadedV2Ys, unshadedV2Zs, unshadedV2Ws, shadedV2Xs, shadedV2Ys, shadedV2Zs, shadedV2Ws);
			}

			int &writeIndex = vertexShaderOutputCache.triangleWriteCount;
			DebugAssert(writeIndex < MAX_DRAW_CALL_MESH_TRIANGLES);

			auto &resultV0XYZW = vertexShaderOutputCache.shadedV0XYZWArray[writeIndex];
			auto &resultV1XYZW = vertexShaderOutputCache.shadedV1XYZWArray[writeIndex];
			auto &resultV2XYZW = vertexShaderOutputCache.shadedV2XYZWArray[writeIndex];
			auto &resultUV0XY = vertexShaderOutputCache.uv0XYArray[writeIndex];
			auto &resultUV1XY = vertexShaderOutputCache.uv1XYArray[writeIndex];
			auto &resultUV2XY = vertexShaderOutputCache.uv2XYArray[writeIndex];
			resultV0XYZW[0] = shadedV0Xs[0];
			resultV0XYZW[1] = shadedV0Ys[0];
			resultV0XYZW[2] = shadedV0Zs[0];
			resultV0XYZW[3] = shadedV0Ws[0];
			resultV1XYZW[0] = shadedV1Xs[0];
			resultV1XYZW[1] = shadedV1Ys[0];
			resultV1XYZW[2] = shadedV1Zs[0];
			resultV1XYZW[3] = shadedV1Ws[0];
			resultV2XYZW[0] = shadedV2Xs[0];
			resultV2XYZW[1] = shadedV2Ys[0];
			resultV2XYZW[2] = shadedV2Zs[0];
			resultV2XYZW[3] = shadedV2Ws[0];
			resultUV0XY[0] = vertexShaderInputCache.uv0Xs[triangleIndex];
			resultUV0XY[1] = vertexShaderInputCache.uv0Ys[triangleIndex];
			resultUV1XY[0] = vertexShaderInputCache.uv1Xs[triangleIndex];
			resultUV1XY[1] = vertexShaderInputCache.uv1Ys[triangleIndex];
			resultUV2XY[0] = vertexShaderInputCache.uv2Xs[triangleIndex];
			resultUV2XY[1] = vertexShaderInputCache.uv2Ys[triangleIndex];
			writeIndex++;
			triangleIndex++;
		}
	}

	// Operates on the current sequence of draw call meshes with the chosen vertex shader then writes results
	// to a cache for mesh clipping.
	void ProcessVertexShaders(VertexShaderType vertexShaderType, const TransformCache &transformCache, const VertexShaderInputCache &vertexShaderInputCache,
		VertexShaderOutputCache &vertexShaderOutputCache)
	{
		// Dispatch based on vertex shader.
		switch (vertexShaderType)
		{
		case VertexShaderType::Basic:
			ProcessVertexShadersInternal<VertexShaderType::Basic>(transformCache, vertexShaderInputCache, vertexShaderOutputCache);
			break;
		case VertexShaderType::RaisingDoor:
			ProcessVertexShadersInternal<VertexShaderType::RaisingDoor>(transformCache, vertexShaderInputCache, vertexShaderOutputCache);
			break;
		case VertexShaderType::Entity:
			ProcessVertexShadersInternal<VertexShaderType::Entity>(transformCache, vertexShaderInputCache, vertexShaderOutputCache);
			break;
		default:
			DebugNotImplementedMsg(std::to_string(static_cast<int>(vertexShaderType)));
			break;
		}
	}

	template<int clipPlaneIndex>
	void ProcessClippingWithPlane(ClippingOutputCache &clippingOutputCache, int &clipListSize, int &clipListFrontIndex)
	{
		auto &clipSpaceTriangleV0XYZWs = clippingOutputCache.clipSpaceTriangleV0XYZWArray;
		auto &clipSpaceTriangleV1XYZWs = clippingOutputCache.clipSpaceTriangleV1XYZWArray;
		auto &clipSpaceTriangleV2XYZWs = clippingOutputCache.clipSpaceTriangleV2XYZWArray;
		auto &clipSpaceTriangleUV0XYs = clippingOutputCache.clipSpaceTriangleUV0XYArray;
		auto &clipSpaceTriangleUV1XYs = clippingOutputCache.clipSpaceTriangleUV1XYArray;
		auto &clipSpaceTriangleUV2XYs = clippingOutputCache.clipSpaceTriangleUV2XYArray;

		const int trianglesToClipCount = clipListSize - clipListFrontIndex;
		for (int triangleToClip = trianglesToClipCount; triangleToClip > 0; triangleToClip--)
		{
			const auto &clipSpaceTriangleV0XYZW = clipSpaceTriangleV0XYZWs[clipListFrontIndex];
			const auto &clipSpaceTriangleV1XYZW = clipSpaceTriangleV1XYZWs[clipListFrontIndex];
			const auto &clipSpaceTriangleV2XYZW = clipSpaceTriangleV2XYZWs[clipListFrontIndex];
			const auto &clipSpaceTriangleUV0XY = clipSpaceTriangleUV0XYs[clipListFrontIndex];
			const auto &clipSpaceTriangleUV1XY = clipSpaceTriangleUV1XYs[clipListFrontIndex];
			const auto &clipSpaceTriangleUV2XY = clipSpaceTriangleUV2XYs[clipListFrontIndex];

			// Active vertices for clipping. The last two are populated below if clipping is needed.
			double currentVXs[] = { clipSpaceTriangleV0XYZW[0], clipSpaceTriangleV1XYZW[0], clipSpaceTriangleV2XYZW[0], 0.0, 0.0 };
			double currentVYs[] = { clipSpaceTriangleV0XYZW[1], clipSpaceTriangleV1XYZW[1], clipSpaceTriangleV2XYZW[1], 0.0, 0.0 };
			double currentVZs[] = { clipSpaceTriangleV0XYZW[2], clipSpaceTriangleV1XYZW[2], clipSpaceTriangleV2XYZW[2], 0.0, 0.0 };
			double currentVWs[] = { clipSpaceTriangleV0XYZW[3], clipSpaceTriangleV1XYZW[3], clipSpaceTriangleV2XYZW[3], 0.0, 0.0 };
			constexpr int GENERATED_RESULT_INDEX0 = 3;
			constexpr int GENERATED_RESULT_INDEX1 = 4;

			double v0Component, v1Component, v2Component;
			if constexpr ((clipPlaneIndex == 0) || (clipPlaneIndex == 1))
			{
				v0Component = currentVXs[0];
				v1Component = currentVXs[1];
				v2Component = currentVXs[2];
			}
			else if ((clipPlaneIndex == 2) || (clipPlaneIndex == 3))
			{
				v0Component = currentVYs[0];
				v1Component = currentVYs[1];
				v2Component = currentVYs[2];
			}
			else
			{
				v0Component = currentVZs[0];
				v1Component = currentVZs[1];
				v2Component = currentVZs[2];
			}

			double v0w, v1w, v2w;
			double comparisonSign;
			if constexpr ((clipPlaneIndex & 1) == 0)
			{
				v0w = currentVWs[0];
				v1w = currentVWs[1];
				v2w = currentVWs[2];
				comparisonSign = 1.0;
			}
			else
			{
				v0w = -currentVWs[0];
				v1w = -currentVWs[1];
				v2w = -currentVWs[2];
				comparisonSign = -1.0;
			}

			const double vDiffs[] = { v0Component + v0w, v1Component + v1w, v2Component + v2w };
			const bool isV0Inside = (vDiffs[0] * comparisonSign) >= 0.0;
			const bool isV1Inside = (vDiffs[1] * comparisonSign) >= 0.0;
			const bool isV2Inside = (vDiffs[2] * comparisonSign) >= 0.0;

			// Active texture coordinates for clipping, same rule as vertices above.
			double currentUVXs[] = { clipSpaceTriangleUV0XY[0], clipSpaceTriangleUV1XY[0], clipSpaceTriangleUV2XY[0], 0.0, 0.0 };
			double currentUVYs[] = { clipSpaceTriangleUV0XY[1], clipSpaceTriangleUV1XY[1], clipSpaceTriangleUV2XY[1], 0.0, 0.0 };

			const int resultWriteIndex0 = clipListSize;
			const int resultWriteIndex1 = clipListSize + 1;
			auto &result0V0XYZW = clipSpaceTriangleV0XYZWs[resultWriteIndex0];
			auto &result0V1XYZW = clipSpaceTriangleV1XYZWs[resultWriteIndex0];
			auto &result0V2XYZW = clipSpaceTriangleV2XYZWs[resultWriteIndex0];
			auto &result1V0XYZW = clipSpaceTriangleV0XYZWs[resultWriteIndex1];
			auto &result1V1XYZW = clipSpaceTriangleV1XYZWs[resultWriteIndex1];
			auto &result1V2XYZW = clipSpaceTriangleV2XYZWs[resultWriteIndex1];
			auto &result0UV0XY = clipSpaceTriangleUV0XYs[resultWriteIndex0];
			auto &result0UV1XY = clipSpaceTriangleUV1XYs[resultWriteIndex0];
			auto &result0UV2XY = clipSpaceTriangleUV2XYs[resultWriteIndex0];
			auto &result1UV0XY = clipSpaceTriangleUV0XYs[resultWriteIndex1];
			auto &result1UV1XY = clipSpaceTriangleUV1XYs[resultWriteIndex1];
			auto &result1UV2XY = clipSpaceTriangleUV2XYs[resultWriteIndex1];

			const int insideMaskIndex = (isV2Inside ? 0 : 1) | (isV1Inside ? 0 : 2) | (isV0Inside ? 0 : 4);
			constexpr int clipCaseResultTriangleCounts[] =
			{
				1, // All three input vertices visible
				2, // Becomes quad (Inside: V0, V1. Outside: V2)
				2, // Becomes quad (Inside: V0, V2. Outside: V1)
				1, // Becomes smaller triangle (Inside: V0. Outside: V1, V2)
				2, // Becomes quad (Inside: V1, V2. Outside: V0)
				1, // Becomes smaller triangle (Inside: V1. Outside: V0, V2)
				1, // Becomes smaller triangle (Inside: V2. Outside: V0, V1)
				0 // No input vertices visible
			};

			const int clipResultCount = clipCaseResultTriangleCounts[insideMaskIndex];
			const bool becomesQuad = clipResultCount == 2;

			if (insideMaskIndex == 0)
			{
				// All vertices visible, no clipping needed.
				result0V0XYZW[0] = currentVXs[0];
				result0V0XYZW[1] = currentVYs[0];
				result0V0XYZW[2] = currentVZs[0];
				result0V0XYZW[3] = currentVWs[0];
				result0V1XYZW[0] = currentVXs[1];
				result0V1XYZW[1] = currentVYs[1];
				result0V1XYZW[2] = currentVZs[1];
				result0V1XYZW[3] = currentVWs[1];
				result0V2XYZW[0] = currentVXs[2];
				result0V2XYZW[1] = currentVYs[2];
				result0V2XYZW[2] = currentVZs[2];
				result0V2XYZW[3] = currentVWs[2];
				result0UV0XY[0] = currentUVXs[0];
				result0UV0XY[1] = currentUVYs[0];
				result0UV1XY[0] = currentUVXs[1];
				result0UV1XY[1] = currentUVYs[1];
				result0UV2XY[0] = currentUVXs[2];
				result0UV2XY[1] = currentUVYs[2];
			}
			else if (insideMaskIndex == 7)
			{
				// All three vertices outside frustum, write nothing.
			}
			else
			{
				// Determine which two line segments are intersecting the clipping plane. The input and result
				// vertex orders depend on the clip case.
				int inputIndex0, inputIndex1, inputIndex2, inputIndex3;
				int resultIndex0, resultIndex1, resultIndex2, resultIndex3, resultIndex4, resultIndex5;
				switch (insideMaskIndex)
				{
				case 1:
					inputIndex0 = 1;
					inputIndex1 = 2;
					inputIndex2 = 2;
					inputIndex3 = 0;
					resultIndex0 = 0;
					resultIndex1 = 1;
					resultIndex2 = 3;
					resultIndex3 = 3;
					resultIndex4 = 4;
					resultIndex5 = 0;
					break;
				case 2:
					inputIndex0 = 0;
					inputIndex1 = 1;
					inputIndex2 = 1;
					inputIndex3 = 2;
					resultIndex0 = 0;
					resultIndex1 = 3;
					resultIndex2 = 4;
					resultIndex3 = 4;
					resultIndex4 = 2;
					resultIndex5 = 0;
					break;
				case 3:
					inputIndex0 = 0;
					inputIndex1 = 1;
					inputIndex2 = 2;
					inputIndex3 = 0;
					resultIndex0 = 0;
					resultIndex1 = 3;
					resultIndex2 = 4;
					break;
				case 4:
					inputIndex0 = 0;
					inputIndex1 = 1;
					inputIndex2 = 2;
					inputIndex3 = 0;
					resultIndex0 = 3;
					resultIndex1 = 1;
					resultIndex2 = 2;
					resultIndex3 = 2;
					resultIndex4 = 4;
					resultIndex5 = 3;
					break;
				case 5:
					inputIndex0 = 0;
					inputIndex1 = 1;
					inputIndex2 = 1;
					inputIndex3 = 2;
					resultIndex0 = 3;
					resultIndex1 = 1;
					resultIndex2 = 4;
					break;
				case 6:
					inputIndex0 = 1;
					inputIndex1 = 2;
					inputIndex2 = 2;
					inputIndex3 = 0;
					resultIndex0 = 3;
					resultIndex1 = 2;
					resultIndex2 = 4;
					break;
				}

				// Calculate distances to clip the two line segments at.
				const double segment0V0Diff = vDiffs[inputIndex0];
				const double segment0V1Diff = vDiffs[inputIndex1];
				const double segment1V0Diff = vDiffs[inputIndex2];
				const double segment1V1Diff = vDiffs[inputIndex3];
				const double segment0PointT = segment0V0Diff / (segment0V0Diff - segment0V1Diff);
				const double segment1PointT = segment1V0Diff / (segment1V0Diff - segment1V1Diff);

				// Generate two vertices and texture coordinates, making sure to keep the original winding order.
				Double_LerpN<1>(currentVXs + inputIndex0, currentVXs + inputIndex1, &segment0PointT, currentVXs + GENERATED_RESULT_INDEX0);
				Double_LerpN<1>(currentVYs + inputIndex0, currentVYs + inputIndex1, &segment0PointT, currentVYs + GENERATED_RESULT_INDEX0);
				Double_LerpN<1>(currentVZs + inputIndex0, currentVZs + inputIndex1, &segment0PointT, currentVZs + GENERATED_RESULT_INDEX0);
				Double_LerpN<1>(currentVWs + inputIndex0, currentVWs + inputIndex1, &segment0PointT, currentVWs + GENERATED_RESULT_INDEX0);
				Double_LerpN<1>(currentVXs + inputIndex2, currentVXs + inputIndex3, &segment1PointT, currentVXs + GENERATED_RESULT_INDEX1);
				Double_LerpN<1>(currentVYs + inputIndex2, currentVYs + inputIndex3, &segment1PointT, currentVYs + GENERATED_RESULT_INDEX1);
				Double_LerpN<1>(currentVZs + inputIndex2, currentVZs + inputIndex3, &segment1PointT, currentVZs + GENERATED_RESULT_INDEX1);
				Double_LerpN<1>(currentVWs + inputIndex2, currentVWs + inputIndex3, &segment1PointT, currentVWs + GENERATED_RESULT_INDEX1);
				Double_LerpN<1>(currentUVXs + inputIndex0, currentUVXs + inputIndex1, &segment0PointT, currentUVXs + GENERATED_RESULT_INDEX0);
				Double_LerpN<1>(currentUVYs + inputIndex0, currentUVYs + inputIndex1, &segment0PointT, currentUVYs + GENERATED_RESULT_INDEX0);
				Double_LerpN<1>(currentUVXs + inputIndex2, currentUVXs + inputIndex3, &segment1PointT, currentUVXs + GENERATED_RESULT_INDEX1);
				Double_LerpN<1>(currentUVYs + inputIndex2, currentUVYs + inputIndex3, &segment1PointT, currentUVYs + GENERATED_RESULT_INDEX1);

				result0V0XYZW[0] = currentVXs[resultIndex0];
				result0V0XYZW[1] = currentVYs[resultIndex0];
				result0V0XYZW[2] = currentVZs[resultIndex0];
				result0V0XYZW[3] = currentVWs[resultIndex0];
				result0V1XYZW[0] = currentVXs[resultIndex1];
				result0V1XYZW[1] = currentVYs[resultIndex1];
				result0V1XYZW[2] = currentVZs[resultIndex1];
				result0V1XYZW[3] = currentVWs[resultIndex1];
				result0V2XYZW[0] = currentVXs[resultIndex2];
				result0V2XYZW[1] = currentVYs[resultIndex2];
				result0V2XYZW[2] = currentVZs[resultIndex2];
				result0V2XYZW[3] = currentVWs[resultIndex2];
				result0UV0XY[0] = currentUVXs[resultIndex0];
				result0UV0XY[1] = currentUVYs[resultIndex0];
				result0UV1XY[0] = currentUVXs[resultIndex1];
				result0UV1XY[1] = currentUVYs[resultIndex1];
				result0UV2XY[0] = currentUVXs[resultIndex2];
				result0UV2XY[1] = currentUVYs[resultIndex2];

				if (becomesQuad)
				{
					result1V0XYZW[0] = currentVXs[resultIndex3];
					result1V0XYZW[1] = currentVYs[resultIndex3];
					result1V0XYZW[2] = currentVZs[resultIndex3];
					result1V0XYZW[3] = currentVWs[resultIndex3];
					result1V1XYZW[0] = currentVXs[resultIndex4];
					result1V1XYZW[1] = currentVYs[resultIndex4];
					result1V1XYZW[2] = currentVZs[resultIndex4];
					result1V1XYZW[3] = currentVWs[resultIndex4];
					result1V2XYZW[0] = currentVXs[resultIndex5];
					result1V2XYZW[1] = currentVYs[resultIndex5];
					result1V2XYZW[2] = currentVZs[resultIndex5];
					result1V2XYZW[3] = currentVWs[resultIndex5];
					result1UV0XY[0] = currentUVXs[resultIndex3];
					result1UV0XY[1] = currentUVYs[resultIndex3];
					result1UV1XY[0] = currentUVXs[resultIndex4];
					result1UV1XY[1] = currentUVYs[resultIndex4];
					result1UV2XY[0] = currentUVXs[resultIndex5];
					result1UV2XY[1] = currentUVYs[resultIndex5];
				}
			}

			clipListSize += clipResultCount;
			clipListFrontIndex++;
		}
	}

	// Clips triangles to the frustum then writes out clip space triangle indices for the rasterizer to iterate.
	void ProcessClipping(const DrawCallCache &drawCallCache, const VertexShaderOutputCache &vertexShaderOutputCache, ClippingOutputCache &clippingOutputCache)
	{
		const auto &shadedV0XYZWs = vertexShaderOutputCache.shadedV0XYZWArray;
		const auto &shadedV1XYZWs = vertexShaderOutputCache.shadedV1XYZWArray;
		const auto &shadedV2XYZWs = vertexShaderOutputCache.shadedV2XYZWArray;
		const auto &uv0XYs = vertexShaderOutputCache.uv0XYArray;
		const auto &uv1XYs = vertexShaderOutputCache.uv1XYArray;
		const auto &uv2XYs = vertexShaderOutputCache.uv2XYArray;
		auto &clipSpaceTriangleV0XYZWs = clippingOutputCache.clipSpaceTriangleV0XYZWArray;
		auto &clipSpaceTriangleV1XYZWs = clippingOutputCache.clipSpaceTriangleV1XYZWArray;
		auto &clipSpaceTriangleV2XYZWs = clippingOutputCache.clipSpaceTriangleV2XYZWArray;
		auto &clipSpaceTriangleUV0XYs = clippingOutputCache.clipSpaceTriangleUV0XYArray;
		auto &clipSpaceTriangleUV1XYs = clippingOutputCache.clipSpaceTriangleUV1XYArray;
		auto &clipSpaceTriangleUV2XYs = clippingOutputCache.clipSpaceTriangleUV2XYArray;
		auto &clipSpaceMeshV0XYZWs = clippingOutputCache.clipSpaceMeshV0XYZWArray;
		auto &clipSpaceMeshV1XYZWs = clippingOutputCache.clipSpaceMeshV1XYZWArray;
		auto &clipSpaceMeshV2XYZWs = clippingOutputCache.clipSpaceMeshV2XYZWArray;
		auto &clipSpaceMeshUV0XYs = clippingOutputCache.clipSpaceMeshUV0XYArray;
		auto &clipSpaceMeshUV1XYs = clippingOutputCache.clipSpaceMeshUV1XYArray;
		auto &clipSpaceMeshUV2XYs = clippingOutputCache.clipSpaceMeshUV2XYArray;
		int &clipSpaceMeshTriangleCount = clippingOutputCache.clipSpaceMeshTriangleCount;

		// Reset clip space cache. Skip zeroing the mesh arrays for performance.
		clipSpaceMeshTriangleCount = 0;

		// Clip each vertex-shaded triangle and save them in a cache for rasterization.
		const int triangleCount = drawCallCache.indexBuffer->triangleCount;
		for (int triangleIndex = 0; triangleIndex < triangleCount; triangleIndex++)
		{
			const auto &shadedV0XYZW = shadedV0XYZWs[triangleIndex];
			const auto &shadedV1XYZW = shadedV1XYZWs[triangleIndex];
			const auto &shadedV2XYZW = shadedV2XYZWs[triangleIndex];
			const auto &uv0XY = uv0XYs[triangleIndex];
			const auto &uv1XY = uv1XYs[triangleIndex];
			const auto &uv2XY = uv2XYs[triangleIndex];
			auto &firstClipSpaceTriangleV0XYZW = clipSpaceTriangleV0XYZWs[0];
			auto &firstClipSpaceTriangleV1XYZW = clipSpaceTriangleV1XYZWs[0];
			auto &firstClipSpaceTriangleV2XYZW = clipSpaceTriangleV2XYZWs[0];
			auto &firstClipSpaceTriangleUV0XY = clipSpaceTriangleUV0XYs[0];
			auto &firstClipSpaceTriangleUV1XY = clipSpaceTriangleUV1XYs[0];
			auto &firstClipSpaceTriangleUV2XY = clipSpaceTriangleUV2XYs[0];

			// Initialize clipping loop with the vertex-shaded triangle.
			firstClipSpaceTriangleV0XYZW[0] = shadedV0XYZW[0];
			firstClipSpaceTriangleV0XYZW[1] = shadedV0XYZW[1];
			firstClipSpaceTriangleV0XYZW[2] = shadedV0XYZW[2];
			firstClipSpaceTriangleV0XYZW[3] = shadedV0XYZW[3];
			firstClipSpaceTriangleV1XYZW[0] = shadedV1XYZW[0];
			firstClipSpaceTriangleV1XYZW[1] = shadedV1XYZW[1];
			firstClipSpaceTriangleV1XYZW[2] = shadedV1XYZW[2];
			firstClipSpaceTriangleV1XYZW[3] = shadedV1XYZW[3];
			firstClipSpaceTriangleV2XYZW[0] = shadedV2XYZW[0];
			firstClipSpaceTriangleV2XYZW[1] = shadedV2XYZW[1];
			firstClipSpaceTriangleV2XYZW[2] = shadedV2XYZW[2];
			firstClipSpaceTriangleV2XYZW[3] = shadedV2XYZW[3];
			firstClipSpaceTriangleUV0XY[0] = uv0XY[0];
			firstClipSpaceTriangleUV0XY[1] = uv0XY[1];
			firstClipSpaceTriangleUV1XY[0] = uv1XY[0];
			firstClipSpaceTriangleUV1XY[1] = uv1XY[1];
			firstClipSpaceTriangleUV2XY[0] = uv2XY[0];
			firstClipSpaceTriangleUV2XY[1] = uv2XY[1];

			int clipListSize = 1; // Triangles to process based on this vertex-shaded triangle.
			int clipListFrontIndex = 0;

			// Check each dimension against -W and W components.
			ProcessClippingWithPlane<0>(clippingOutputCache, clipListSize, clipListFrontIndex);
			ProcessClippingWithPlane<1>(clippingOutputCache, clipListSize, clipListFrontIndex);
			ProcessClippingWithPlane<2>(clippingOutputCache, clipListSize, clipListFrontIndex);
			ProcessClippingWithPlane<3>(clippingOutputCache, clipListSize, clipListFrontIndex);
			ProcessClippingWithPlane<4>(clippingOutputCache, clipListSize, clipListFrontIndex);
			ProcessClippingWithPlane<5>(clippingOutputCache, clipListSize, clipListFrontIndex);

			// Add the clip results to the mesh, skipping the incomplete triangles the front index advanced beyond.
			const int resultTriangleCount = clipListSize - clipListFrontIndex;
			for (int resultTriangleIndex = 0; resultTriangleIndex < resultTriangleCount; resultTriangleIndex++)
			{
				const int srcIndex = clipListFrontIndex + resultTriangleIndex;
				const auto &clipSpaceTriangleV0XYZW = clipSpaceTriangleV0XYZWs[srcIndex];
				const auto &clipSpaceTriangleV1XYZW = clipSpaceTriangleV1XYZWs[srcIndex];
				const auto &clipSpaceTriangleV2XYZW = clipSpaceTriangleV2XYZWs[srcIndex];
				const auto &clipSpaceTriangleUV0XY = clipSpaceTriangleUV0XYs[srcIndex];
				const auto &clipSpaceTriangleUV1XY = clipSpaceTriangleUV1XYs[srcIndex];
				const auto &clipSpaceTriangleUV2XY = clipSpaceTriangleUV2XYs[srcIndex];

				const int dstIndex = clipSpaceMeshTriangleCount + resultTriangleIndex;
				auto &clipSpaceMeshV0XYZW = clipSpaceMeshV0XYZWs[dstIndex];
				auto &clipSpaceMeshV1XYZW = clipSpaceMeshV1XYZWs[dstIndex];
				auto &clipSpaceMeshV2XYZW = clipSpaceMeshV2XYZWs[dstIndex];
				auto &clipSpaceMeshUV0XY = clipSpaceMeshUV0XYs[dstIndex];
				auto &clipSpaceMeshUV1XY = clipSpaceMeshUV1XYs[dstIndex];
				auto &clipSpaceMeshUV2XY = clipSpaceMeshUV2XYs[dstIndex];

				clipSpaceMeshV0XYZW[0] = clipSpaceTriangleV0XYZW[0];
				clipSpaceMeshV0XYZW[1] = clipSpaceTriangleV0XYZW[1];
				clipSpaceMeshV0XYZW[2] = clipSpaceTriangleV0XYZW[2];
				clipSpaceMeshV0XYZW[3] = clipSpaceTriangleV0XYZW[3];
				clipSpaceMeshV1XYZW[0] = clipSpaceTriangleV1XYZW[0];
				clipSpaceMeshV1XYZW[1] = clipSpaceTriangleV1XYZW[1];
				clipSpaceMeshV1XYZW[2] = clipSpaceTriangleV1XYZW[2];
				clipSpaceMeshV1XYZW[3] = clipSpaceTriangleV1XYZW[3];
				clipSpaceMeshV2XYZW[0] = clipSpaceTriangleV2XYZW[0];
				clipSpaceMeshV2XYZW[1] = clipSpaceTriangleV2XYZW[1];
				clipSpaceMeshV2XYZW[2] = clipSpaceTriangleV2XYZW[2];
				clipSpaceMeshV2XYZW[3] = clipSpaceTriangleV2XYZW[3];
				clipSpaceMeshUV0XY[0] = clipSpaceTriangleUV0XY[0];
				clipSpaceMeshUV0XY[1] = clipSpaceTriangleUV0XY[1];
				clipSpaceMeshUV1XY[0] = clipSpaceTriangleUV1XY[0];
				clipSpaceMeshUV1XY[1] = clipSpaceTriangleUV1XY[1];
				clipSpaceMeshUV2XY[0] = clipSpaceTriangleUV2XY[0];
				clipSpaceMeshUV2XY[1] = clipSpaceTriangleUV2XY[1];
			}

			clipSpaceMeshTriangleCount += resultTriangleCount;
		}
	}
}

// Rasterizer bin types.
namespace
{
	static constexpr int MAX_WORKER_DRAW_CALLS_PER_LOOP = 8192; // Number of draw calls a worker can process each thread sync.

	struct RasterizerWorkItem
	{
		int binX, binY, binIndex;

		RasterizerWorkItem()
		{
			this->binX = -1;
			this->binY = -1;
			this->binIndex = -1;
		}

		RasterizerWorkItem(int binX, int binY, int binIndex)
		{
			this->binX = binX;
			this->binY = binY;
			this->binIndex = binIndex;
		}
	};

	// A selection of triangle indices in a mesh tied to one of the worker's draw calls.
	struct RasterizerBinEntry
	{
		int workerDrawCallIndex;
		int triangleIndicesStartIndex, triangleIndicesCount; // Range of indices in the bin's indices to rasterize.

		RasterizerBinEntry()
		{
			this->clear();
		}

		void clear()
		{
			this->workerDrawCallIndex = -1;
			this->triangleIndicesStartIndex = -1;
			this->triangleIndicesCount = 0;
		}

		void init(int workerDrawCallIndex, int triangleIndicesStartIndex)
		{
			this->workerDrawCallIndex = workerDrawCallIndex;
			this->triangleIndicesStartIndex = triangleIndicesStartIndex;
			this->triangleIndicesCount = 0;
		}
	};
}

// Each bin points to front-facing triangles that at least partially touch a screen-space tile
// (has to be outside a namespace due to being in SoftwareRenderer).
struct RasterizerBin
{
	static constexpr int MAX_FRUSTUM_TRIANGLES = 16384;

	RasterizerBinEntry entries[MAX_WORKER_DRAW_CALLS_PER_LOOP]; // Draw call index + the portion of a mesh pointing into triangleIndicesToRasterize.
	int entryCount;

	int triangleIndicesToRasterize[MAX_FRUSTUM_TRIANGLES]; // Points into this worker's triangles to rasterize.
	int triangleBinPixelXStarts[MAX_FRUSTUM_TRIANGLES];
	int triangleBinPixelXEnds[MAX_FRUSTUM_TRIANGLES];
	int triangleBinPixelYStarts[MAX_FRUSTUM_TRIANGLES];
	int triangleBinPixelYEnds[MAX_FRUSTUM_TRIANGLES];
	int triangleCount; // Triangles this bin should try to render. Determines where the next bin entry can allocate its triangle range.

	RasterizerBin()
	{
		this->clear();
	}

	void clear()
	{
		this->entryCount = 0;
		this->triangleCount = 0;
	}

	RasterizerBinEntry &getOrAddEntry(int workerDrawCallIndex, int triangleIndicesStartIndex)
	{
		DebugAssert(workerDrawCallIndex >= 0);
		DebugAssert(workerDrawCallIndex < MAX_WORKER_DRAW_CALLS_PER_LOOP);
		DebugAssert(triangleIndicesStartIndex >= 0);
		DebugAssert(triangleIndicesStartIndex < (MAX_FRUSTUM_TRIANGLES - 1));

		RasterizerBinEntry *entryPtr = nullptr;
		for (int i = 0; i < this->entryCount; i++)
		{
			RasterizerBinEntry &curEntry = this->entries[i];
			if (curEntry.workerDrawCallIndex == workerDrawCallIndex)
			{
				entryPtr = &curEntry;
				break;
			}
		}

		if (entryPtr == nullptr)
		{
			DebugAssertMsg(this->entryCount < std::size(this->entries), "Too many bin entries, can't insert for worker draw call index " + std::to_string(workerDrawCallIndex) + ".");
			entryPtr = &this->entries[this->entryCount];
			entryPtr->init(workerDrawCallIndex, triangleIndicesStartIndex);
			this->entryCount++;
		}

		return *entryPtr;
	}
};

// Rasterizer, pixel shader execution.
namespace
{
	struct RasterizerInputCache
	{
		static constexpr int MAX_FRUSTUM_TRIANGLES = 16384;

		RasterizerTriangle triangles[MAX_FRUSTUM_TRIANGLES]; // @todo: ideally this triangles array would still be global for all threads; lots of memory waste currently
		int triangleCount;

		Buffer2D<RasterizerBin> bins;
		int binWidth, binHeight;
		int binCountX, binCountY;

		RasterizerInputCache()
		{
			this->triangleCount = 0;
			this->binWidth = 0;
			this->binHeight = 0;
			this->binCountX = 0;
			this->binCountY = 0;
		}

		void clearTriangles()
		{
			this->triangleCount = 0;
		}

		void createBins(int frameBufferWidth, int frameBufferHeight)
		{
			this->binWidth = GetRasterizerBinWidth(frameBufferWidth);
			this->binHeight = GetRasterizerBinHeight(frameBufferHeight);
			this->binCountX = GetRasterizerBinCountX(frameBufferWidth, this->binWidth);
			this->binCountY = GetRasterizerBinCountY(frameBufferHeight, this->binHeight);
			this->bins.init(this->binCountX, this->binCountY);
		}

		void emptyBins()
		{
			for (RasterizerBin &bin : this->bins)
			{
				bin.clear();
			}
		}
	};

	void ProcessClipSpaceTrianglesForBinning(int workerDrawCallIndex, const ClippingOutputCache &clippingOutputCache, RasterizerInputCache &rasterizerInputCache)
	{
		const auto &clipSpaceMeshV0XYZWs = clippingOutputCache.clipSpaceMeshV0XYZWArray;
		const auto &clipSpaceMeshV1XYZWs = clippingOutputCache.clipSpaceMeshV1XYZWArray;
		const auto &clipSpaceMeshV2XYZWs = clippingOutputCache.clipSpaceMeshV2XYZWArray;
		const auto &clipSpaceMeshUV0XYs = clippingOutputCache.clipSpaceMeshUV0XYArray;
		const auto &clipSpaceMeshUV1XYs = clippingOutputCache.clipSpaceMeshUV1XYArray;
		const auto &clipSpaceMeshUV2XYs = clippingOutputCache.clipSpaceMeshUV2XYArray;

		const int meshTriangleCount = clippingOutputCache.clipSpaceMeshTriangleCount;
		for (int meshTriangleIndex = 0; meshTriangleIndex < meshTriangleCount; meshTriangleIndex++)
		{
			const auto &clipSpaceMeshV0XYZW = clipSpaceMeshV0XYZWs[meshTriangleIndex];
			const auto &clipSpaceMeshV1XYZW = clipSpaceMeshV1XYZWs[meshTriangleIndex];
			const auto &clipSpaceMeshV2XYZW = clipSpaceMeshV2XYZWs[meshTriangleIndex];
			const double clip0X = clipSpaceMeshV0XYZW[0];
			const double clip0Y = clipSpaceMeshV0XYZW[1];
			const double clip0Z = clipSpaceMeshV0XYZW[2];
			const double clip0W = clipSpaceMeshV0XYZW[3];
			const double clip1X = clipSpaceMeshV1XYZW[0];
			const double clip1Y = clipSpaceMeshV1XYZW[1];
			const double clip1Z = clipSpaceMeshV1XYZW[2];
			const double clip1W = clipSpaceMeshV1XYZW[3];
			const double clip2X = clipSpaceMeshV2XYZW[0];
			const double clip2Y = clipSpaceMeshV2XYZW[1];
			const double clip2Z = clipSpaceMeshV2XYZW[2];
			const double clip2W = clipSpaceMeshV2XYZW[3];
			const double clip0WRecip = 1.0 / clip0W;
			const double clip1WRecip = 1.0 / clip1W;
			const double clip2WRecip = 1.0 / clip2W;
			const double ndc0X = clip0X * clip0WRecip;
			const double ndc0Y = clip0Y * clip0WRecip;
			const double ndc0Z = clip0Z * clip0WRecip;
			const double ndc1X = clip1X * clip1WRecip;
			const double ndc1Y = clip1Y * clip1WRecip;
			const double ndc1Z = clip1Z * clip1WRecip;
			const double ndc2X = clip2X * clip2WRecip;
			const double ndc2Y = clip2Y * clip2WRecip;
			const double ndc2Z = clip2Z * clip2WRecip;
			const double screenSpace0X = NdcXToScreenSpace(ndc0X, g_frameBufferWidthReal);
			const double screenSpace0Y = NdcYToScreenSpace(ndc0Y, g_frameBufferHeightReal);
			const double screenSpace1X = NdcXToScreenSpace(ndc1X, g_frameBufferWidthReal);
			const double screenSpace1Y = NdcYToScreenSpace(ndc1Y, g_frameBufferHeightReal);
			const double screenSpace2X = NdcXToScreenSpace(ndc2X, g_frameBufferWidthReal);
			const double screenSpace2Y = NdcYToScreenSpace(ndc2Y, g_frameBufferHeightReal);
			const double screenSpace01X = screenSpace1X - screenSpace0X;
			const double screenSpace01Y = screenSpace1Y - screenSpace0Y;
			const double screenSpace12X = screenSpace2X - screenSpace1X;
			const double screenSpace12Y = screenSpace2Y - screenSpace1Y;
			const double screenSpace20X = screenSpace0X - screenSpace2X;
			const double screenSpace20Y = screenSpace0Y - screenSpace2Y;

			double screenSpace01Cross12, screenSpace12Cross20, screenSpace20Cross01;
			Double2_CrossN<1>(&screenSpace12X, &screenSpace12Y, &screenSpace01X, &screenSpace01Y, &screenSpace01Cross12);
			Double2_CrossN<1>(&screenSpace20X, &screenSpace20Y, &screenSpace12X, &screenSpace12Y, &screenSpace12Cross20);
			Double2_CrossN<1>(&screenSpace01X, &screenSpace01Y, &screenSpace20X, &screenSpace20Y, &screenSpace20Cross01);

			// Discard back-facing.
			const bool isFrontFacing = (screenSpace01Cross12 + screenSpace12Cross20 + screenSpace20Cross01) > 0.0;
			if (!isFrontFacing)
			{
				continue;
			}

			// Naive screen-space bounding box around triangle.
			const double xMin = std::min(screenSpace0X, std::min(screenSpace1X, screenSpace2X));
			const double xMax = std::max(screenSpace0X, std::max(screenSpace1X, screenSpace2X));
			const double yMin = std::min(screenSpace0Y, std::min(screenSpace1Y, screenSpace2Y));
			const double yMax = std::max(screenSpace0Y, std::max(screenSpace1Y, screenSpace2Y));
			const int xStart = RendererUtils::getLowerBoundedPixel(xMin, g_frameBufferWidth);
			const int xEnd = RendererUtils::getUpperBoundedPixel(xMax, g_frameBufferWidth);
			const int yStart = RendererUtils::getLowerBoundedPixel(yMin, g_frameBufferHeight);
			const int yEnd = RendererUtils::getUpperBoundedPixel(yMax, g_frameBufferHeight);

			const bool hasPositiveScreenArea = (xEnd > xStart) && (yEnd > yStart);
			if (!hasPositiveScreenArea)
			{
				continue;
			}

			double screenSpace01PerpX, screenSpace01PerpY;
			double screenSpace12PerpX, screenSpace12PerpY;
			double screenSpace20PerpX, screenSpace20PerpY;
			Double2_RightPerpN<1>(&screenSpace01X, &screenSpace01Y, &screenSpace01PerpX, &screenSpace01PerpY);
			Double2_RightPerpN<1>(&screenSpace12X, &screenSpace12Y, &screenSpace12PerpX, &screenSpace12PerpY);
			Double2_RightPerpN<1>(&screenSpace20X, &screenSpace20Y, &screenSpace20PerpX, &screenSpace20PerpY);

			const auto &clipSpaceMeshUV0XY = clipSpaceMeshUV0XYs[meshTriangleIndex];
			const auto &clipSpaceMeshUV1XY = clipSpaceMeshUV1XYs[meshTriangleIndex];
			const auto &clipSpaceMeshUV2XY = clipSpaceMeshUV2XYs[meshTriangleIndex];
			const double uv0X = clipSpaceMeshUV0XY[0];
			const double uv0Y = clipSpaceMeshUV0XY[1];
			const double uv1X = clipSpaceMeshUV1XY[0];
			const double uv1Y = clipSpaceMeshUV1XY[1];
			const double uv2X = clipSpaceMeshUV2XY[0];
			const double uv2Y = clipSpaceMeshUV2XY[1];
			const double uv0XDivW = uv0X * clip0WRecip;
			const double uv0YDivW = uv0Y * clip0WRecip;
			const double uv1XDivW = uv1X * clip1WRecip;
			const double uv1YDivW = uv1Y * clip1WRecip;
			const double uv2XDivW = uv2X * clip2WRecip;
			const double uv2YDivW = uv2Y * clip2WRecip;

			// Write triangle to this worker's list.
			auto &outputTriangles = rasterizerInputCache.triangles;
			const int outputTriangleIndex = rasterizerInputCache.triangleCount;
			DebugAssertIndex(outputTriangles, outputTriangleIndex);
			RasterizerTriangle &outputTriangle = outputTriangles[outputTriangleIndex];
			outputTriangle.clip0X = clip0X;
			outputTriangle.clip0Y = clip0Y;
			outputTriangle.clip0Z = clip0Z;
			outputTriangle.clip0W = clip0W;
			outputTriangle.clip1X = clip1X;
			outputTriangle.clip1Y = clip1Y;
			outputTriangle.clip1Z = clip1Z;
			outputTriangle.clip1W = clip1W;
			outputTriangle.clip2X = clip2X;
			outputTriangle.clip2Y = clip2Y;
			outputTriangle.clip2Z = clip2Z;
			outputTriangle.clip2W = clip2W;
			outputTriangle.clip0WRecip = clip0WRecip;
			outputTriangle.clip1WRecip = clip1WRecip;
			outputTriangle.clip2WRecip = clip2WRecip;
			outputTriangle.ndc0X = ndc0X;
			outputTriangle.ndc0Y = ndc0Y;
			outputTriangle.ndc0Z = ndc0Z;
			outputTriangle.ndc1X = ndc1X;
			outputTriangle.ndc1Y = ndc1Y;
			outputTriangle.ndc1Z = ndc1Z;
			outputTriangle.ndc2X = ndc2X;
			outputTriangle.ndc2Y = ndc2Y;
			outputTriangle.ndc2Z = ndc2Z;
			outputTriangle.screenSpace0X = screenSpace0X;
			outputTriangle.screenSpace0Y = screenSpace0Y;
			outputTriangle.screenSpace1X = screenSpace1X;
			outputTriangle.screenSpace1Y = screenSpace1Y;
			outputTriangle.screenSpace2X = screenSpace2X;
			outputTriangle.screenSpace2Y = screenSpace2Y;
			outputTriangle.screenSpace01X = screenSpace01X;
			outputTriangle.screenSpace01Y = screenSpace01Y;
			outputTriangle.screenSpace12X = screenSpace12X;
			outputTriangle.screenSpace12Y = screenSpace12Y;
			outputTriangle.screenSpace20X = screenSpace20X;
			outputTriangle.screenSpace20Y = screenSpace20Y;
			outputTriangle.screenSpace01PerpX = screenSpace01PerpX;
			outputTriangle.screenSpace01PerpY = screenSpace01PerpY;
			outputTriangle.screenSpace12PerpX = screenSpace12PerpX;
			outputTriangle.screenSpace12PerpY = screenSpace12PerpY;
			outputTriangle.screenSpace20PerpX = screenSpace20PerpX;
			outputTriangle.screenSpace20PerpY = screenSpace20PerpY;
			outputTriangle.uv0X = uv0X;
			outputTriangle.uv0Y = uv0Y;
			outputTriangle.uv1X = uv1X;
			outputTriangle.uv1Y = uv1Y;
			outputTriangle.uv2X = uv2X;
			outputTriangle.uv2Y = uv2Y;
			outputTriangle.uv0XDivW = uv0XDivW;
			outputTriangle.uv0YDivW = uv0YDivW;
			outputTriangle.uv1XDivW = uv1XDivW;
			outputTriangle.uv1YDivW = uv1YDivW;
			outputTriangle.uv2XDivW = uv2XDivW;
			outputTriangle.uv2YDivW = uv2YDivW;

			// Write this triangle's index to all affected rasterizer bins.
			const int binPixelWidth = rasterizerInputCache.binWidth;
			const int binPixelHeight = rasterizerInputCache.binHeight;
			const int startBinX = GetRasterizerBinX(xStart, binPixelWidth);
			const int endBinX = GetRasterizerBinX(xEnd, binPixelWidth);
			const int startBinY = GetRasterizerBinY(yStart, binPixelHeight);
			const int endBinY = GetRasterizerBinY(yEnd, binPixelHeight);
			for (int binY = startBinY; binY <= endBinY; binY++)
			{
				const int binStartFrameBufferPixelY = GetFrameBufferPixelY(binY, 0, binPixelHeight);
				const int binEndFrameBufferPixelY = GetFrameBufferPixelY(binY, binPixelHeight, binPixelHeight);
				const int clampedYStart = std::max(yStart, binStartFrameBufferPixelY);
				const int clampedYEnd = std::min(yEnd, binEndFrameBufferPixelY);
				const int triangleBinPixelYStart = GetRasterizerBinPixelYInclusive(clampedYStart, binPixelHeight);
				const int triangleBinPixelYEnd = GetRasterizerBinPixelYExclusive(clampedYEnd, binPixelHeight);

				for (int binX = startBinX; binX <= endBinX; binX++)
				{
					RasterizerBin &bin = rasterizerInputCache.bins.get(binX, binY);
					const int binTriangleIndex = bin.triangleCount;
					DebugAssert(binTriangleIndex < std::size(bin.triangleIndicesToRasterize));
					bin.triangleIndicesToRasterize[binTriangleIndex] = outputTriangleIndex;

					const int binStartFrameBufferPixelX = GetFrameBufferPixelX(binX, 0, binPixelWidth);
					const int binEndFrameBufferPixelX = GetFrameBufferPixelX(binX, binPixelWidth, binPixelWidth);
					const int clampedXStart = std::max(xStart, binStartFrameBufferPixelX);
					const int clampedXEnd = std::min(xEnd, binEndFrameBufferPixelX);
					bin.triangleBinPixelXStarts[binTriangleIndex] = GetRasterizerBinPixelXInclusive(clampedXStart, binPixelWidth);
					bin.triangleBinPixelXEnds[binTriangleIndex] = GetRasterizerBinPixelXExclusive(clampedXEnd, binPixelWidth);
					bin.triangleBinPixelYStarts[binTriangleIndex] = triangleBinPixelYStart;
					bin.triangleBinPixelYEnds[binTriangleIndex] = triangleBinPixelYEnd;

					RasterizerBinEntry &binEntry = bin.getOrAddEntry(workerDrawCallIndex, binTriangleIndex);
					binEntry.triangleIndicesCount++;
					DebugAssert(binEntry.triangleIndicesCount <= std::size(bin.triangleIndicesToRasterize));

					bin.triangleCount++;
				}
			}

			rasterizerInputCache.triangleCount++;
		}
	}

	void GetWorldSpaceLightIntensityValue(double pointX, double pointY, double pointZ, const SoftwareRenderer::Light &__restrict light,
		double *__restrict outLightIntensity)
	{
		const double lightPointDiffX = light.worldPointX - pointX;
		const double lightPointDiffY = light.worldPointY - pointY;
		const double lightPointDiffZ = light.worldPointZ - pointZ;
		const double lightDistanceSqr = (lightPointDiffX * lightPointDiffX) + (lightPointDiffY * lightPointDiffY) + (lightPointDiffZ * lightPointDiffZ);
		if (lightDistanceSqr <= light.startRadiusSqr)
		{
			*outLightIntensity = 1.0;
		}
		else if (lightDistanceSqr >= light.endRadiusSqr)
		{
			*outLightIntensity = 0.0;
		}
		else
		{
			const double lightDistance = std::sqrt(lightDistanceSqr);
			const double lightDistancePercent = (lightDistance - light.startRadius) * light.startEndRadiusDiffRecip;
			*outLightIntensity = std::clamp(1.0 - lightDistancePercent, 0.0, 1.0);
		}
	}

	template<DitheringMode ditheringMode>
	void GetScreenSpaceDitherValue(double lightLevelReal, double lightIntensitySum, int pixelIndex, bool *__restrict outShouldDither)
	{
		// Dither the light level in screen space.
		if constexpr (ditheringMode == DitheringMode::None)
		{
			*outShouldDither = false;
		}
		else if (ditheringMode == DitheringMode::Classic)
		{
			*outShouldDither = g_ditherBuffer[pixelIndex];
		}
		else if (ditheringMode == DitheringMode::Modern)
		{
			if (lightIntensitySum < 1.0) // Keeps from dithering right next to the camera, not sure why the lowest dither level doesn't do this.
			{
				constexpr int maskCount = DITHERING_MODERN_MASK_COUNT;
				const double lightLevelFraction = lightLevelReal - std::floor(lightLevelReal);
				const int maskIndex = std::clamp(static_cast<int>(static_cast<double>(maskCount) * lightLevelFraction), 0, maskCount - 1);
				const int ditherBufferIndex = pixelIndex + (maskIndex * g_frameBufferPixelCount);
				*outShouldDither = g_ditherBuffer[ditherBufferIndex];
			}
			else
			{
				*outShouldDither = false;
			}
		}
		else
		{
			*outShouldDither = false;
		}
	}

	template<RenderLightingType lightingType, PixelShaderType pixelShaderType, bool enableDepthRead, bool enableDepthWrite, DitheringMode ditheringMode>
	void RasterizeMeshInternal(const DrawCallCache &drawCallCache, const RasterizerInputCache &rasterizerInputCache, const RasterizerBin &bin,
		const RasterizerBinEntry &binEntry, int binX, int binY, int binIndex)
	{
		const double meshLightPercent = drawCallCache.meshLightPercent;
		const auto &lights = drawCallCache.lightPtrArray;
		const int lightCount = drawCallCache.lightCount;
		const double pixelShaderParam0 = drawCallCache.pixelShaderParam0;

		constexpr bool requiresTwoTextures = (pixelShaderType == PixelShaderType::OpaqueWithAlphaTestLayer) || (pixelShaderType == PixelShaderType::OpaqueScreenSpaceAnimationWithAlphaTestLayer) || (pixelShaderType == PixelShaderType::AlphaTestedWithPaletteIndexLookup);
		constexpr bool requiresHorizonMirror = pixelShaderType == PixelShaderType::AlphaTestedWithHorizonMirror;
		constexpr bool requiresPerPixelLightIntensity = lightingType == RenderLightingType::PerPixel;
		constexpr bool requiresPerMeshLightIntensity = lightingType == RenderLightingType::PerMesh;

		PixelShaderLighting shaderLighting;
		shaderLighting.lightTableTexels = g_lightTableTexture->texels8Bit;
		shaderLighting.lightLevelCount = g_lightTableTexture->height;
		shaderLighting.lightLevelCountReal = static_cast<double>(shaderLighting.lightLevelCount);
		shaderLighting.lastLightLevel = shaderLighting.lightLevelCount - 1;
		shaderLighting.texelsPerLightLevel = g_lightTableTexture->width;
		shaderLighting.lightLevel = 0;

		PixelShaderFrameBuffer shaderFrameBuffer;
		shaderFrameBuffer.palette.colors = g_paletteTexture->texels32Bit;
		shaderFrameBuffer.palette.count = g_paletteTexture->texelCount;

		PixelShaderHorizonMirror shaderHorizonMirror;
		if constexpr (requiresHorizonMirror)
		{
			const Double2 horizonScreenSpacePoint = RendererUtils::ndcToScreenSpace(g_horizonNdcPoint, g_frameBufferWidthReal, g_frameBufferHeightReal);
			shaderHorizonMirror.horizonScreenSpacePointX = horizonScreenSpacePoint.x;
			shaderHorizonMirror.horizonScreenSpacePointY = horizonScreenSpacePoint.y;

			DebugAssert(g_skyBgTexture->texelCount > 0);
			shaderHorizonMirror.fallbackSkyColor = g_skyBgTexture->texels8Bit[0];
		}

		const ObjectTextureID textureID0 = drawCallCache.textureID0;
		const ObjectTextureID textureID1 = drawCallCache.textureID1;

		const SoftwareRenderer::ObjectTexture &texture0 = g_objectTextures->get(textureID0);
		PixelShaderTexture shaderTexture0;
		shaderTexture0.init(texture0.texels8Bit, texture0.width, texture0.height);

		PixelShaderTexture shaderTexture1;
		if constexpr (requiresTwoTextures)
		{
			const SoftwareRenderer::ObjectTexture &texture1 = g_objectTextures->get(textureID1);
			shaderTexture1.init(texture1.texels8Bit, texture1.width, texture1.height);
		}

		PixelShaderUniforms shaderUniforms;
		shaderUniforms.screenSpaceAnimPercent = g_screenSpaceAnimPercent;

		// Local variables added to a global afterwards to avoid fighting with threads.
		int totalCoverageTests = 0;
		int totalDepthTests = 0;
		int totalColorWrites = 0;

		const auto &triangleIndices = bin.triangleIndicesToRasterize;
		const auto &triangleBinPixelXStarts = bin.triangleBinPixelXStarts;
		const auto &triangleBinPixelXEnds = bin.triangleBinPixelXEnds;
		const auto &triangleBinPixelYStarts = bin.triangleBinPixelYStarts;
		const auto &triangleBinPixelYEnds = bin.triangleBinPixelYEnds;
		for (int entryTriangleIndex = 0; entryTriangleIndex < binEntry.triangleIndicesCount; entryTriangleIndex++)
		{
			const int triangleIndicesIndex = binEntry.triangleIndicesStartIndex + entryTriangleIndex;
			DebugAssertIndex(triangleIndices, triangleIndicesIndex);
			const int triangleIndex = triangleIndices[triangleIndicesIndex];
			const RasterizerTriangle &triangle = rasterizerInputCache.triangles[triangleIndex];
			const double clip0X = triangle.clip0X;
			const double clip0Y = triangle.clip0Y;
			const double clip0Z = triangle.clip0Z;
			const double clip0W = triangle.clip0W;
			const double clip1X = triangle.clip1X;
			const double clip1Y = triangle.clip1Y;
			const double clip1Z = triangle.clip1Z;
			const double clip1W = triangle.clip1W;
			const double clip2X = triangle.clip2X;
			const double clip2Y = triangle.clip2Y;
			const double clip2Z = triangle.clip2Z;
			const double clip2W = triangle.clip2W;
			const double clip0WRecip = triangle.clip0WRecip;
			const double clip1WRecip = triangle.clip1WRecip;
			const double clip2WRecip = triangle.clip2WRecip;
			const double ndc0X = triangle.ndc0X;
			const double ndc0Y = triangle.ndc0Y;
			const double ndc0Z = triangle.ndc0Z;
			const double ndc1X = triangle.ndc1X;
			const double ndc1Y = triangle.ndc1Y;
			const double ndc1Z = triangle.ndc1Z;
			const double ndc2X = triangle.ndc2X;
			const double ndc2Y = triangle.ndc2Y;
			const double ndc2Z = triangle.ndc2Z;
			const double screenSpace0X = triangle.screenSpace0X;
			const double screenSpace0Y = triangle.screenSpace0Y;
			const double screenSpace1X = triangle.screenSpace1X;
			const double screenSpace1Y = triangle.screenSpace1Y;
			const double screenSpace2X = triangle.screenSpace2X;
			const double screenSpace2Y = triangle.screenSpace2Y;
			const double screenSpace01X = triangle.screenSpace01X;
			const double screenSpace01Y = triangle.screenSpace01Y;
			const double screenSpace12X = triangle.screenSpace12X;
			const double screenSpace12Y = triangle.screenSpace12Y;
			const double screenSpace20X = triangle.screenSpace20X;
			const double screenSpace20Y = triangle.screenSpace20Y;
			const double screenSpace01PerpX = triangle.screenSpace01PerpX;
			const double screenSpace01PerpY = triangle.screenSpace01PerpY;
			const double screenSpace12PerpX = triangle.screenSpace12PerpX;
			const double screenSpace12PerpY = triangle.screenSpace12PerpY;
			const double screenSpace20PerpX = triangle.screenSpace20PerpX;
			const double screenSpace20PerpY = triangle.screenSpace20PerpY;
			const double uv0X = triangle.uv0X;
			const double uv0Y = triangle.uv0Y;
			const double uv1X = triangle.uv1X;
			const double uv1Y = triangle.uv1Y;
			const double uv2X = triangle.uv2X;
			const double uv2Y = triangle.uv2Y;
			const double uv0XDivW = triangle.uv0XDivW;
			const double uv0YDivW = triangle.uv0YDivW;
			const double uv1XDivW = triangle.uv1XDivW;
			const double uv1YDivW = triangle.uv1YDivW;
			const double uv2XDivW = triangle.uv2XDivW;
			const double uv2YDivW = triangle.uv2YDivW;

			const double screenSpace02X = -triangle.screenSpace20X;
			const double screenSpace02Y = -triangle.screenSpace20Y;
			double barycentricDot00, barycentricDot01, barycentricDot11;
			Double2_DotN<1>(&screenSpace01X, &screenSpace01Y, &screenSpace01X, &screenSpace01Y, &barycentricDot00);
			Double2_DotN<1>(&screenSpace01X, &screenSpace01Y, &screenSpace02X, &screenSpace02Y, &barycentricDot01);
			Double2_DotN<1>(&screenSpace02X, &screenSpace02Y, &screenSpace02X, &screenSpace02Y, &barycentricDot11);

			const double barycentricDenominator = (barycentricDot00 * barycentricDot11) - (barycentricDot01 * barycentricDot01);
			const double barycentricDenominatorRecip = 1.0 / barycentricDenominator;

			const int binPixelXStart = bin.triangleBinPixelXStarts[triangleIndicesIndex];
			const int binPixelXEnd = bin.triangleBinPixelXEnds[triangleIndicesIndex];
			const int binPixelYStart = bin.triangleBinPixelYStarts[triangleIndicesIndex];
			const int binPixelYEnd = bin.triangleBinPixelYEnds[triangleIndicesIndex];

			for (int binPixelY = binPixelYStart; binPixelY < binPixelYEnd; binPixelY++)
			{
				const int frameBufferPixelY = GetFrameBufferPixelY(binY, binPixelY, rasterizerInputCache.binHeight);
				shaderFrameBuffer.yPercent = (static_cast<double>(frameBufferPixelY) + 0.50) * g_frameBufferHeightRealRecip;
				const double pixelCenterY = shaderFrameBuffer.yPercent * g_frameBufferHeightReal;
				const double screenSpace0CurrentY = pixelCenterY - screenSpace0Y;
				const double barycentricDot20Y = screenSpace0CurrentY * screenSpace01Y;
				const double barycentricDot21Y = screenSpace0CurrentY * screenSpace02Y;

				double pixelCoverageDot0Y, pixelCoverageDot1Y, pixelCoverageDot2Y;
				GetScreenSpacePointHalfSpaceComponents(pixelCenterY, screenSpace0Y, screenSpace1Y, screenSpace2Y, screenSpace01PerpY,
					screenSpace12PerpY, screenSpace20PerpY, &pixelCoverageDot0Y, &pixelCoverageDot1Y, &pixelCoverageDot2Y);

				for (int binPixelX = binPixelXStart; binPixelX < binPixelXEnd; binPixelX++)
				{
					const int frameBufferPixelX = GetFrameBufferPixelX(binX, binPixelX, rasterizerInputCache.binWidth);
					shaderFrameBuffer.pixelIndex = frameBufferPixelX + (frameBufferPixelY * g_frameBufferWidth);
					shaderFrameBuffer.xPercent = (static_cast<double>(frameBufferPixelX) + 0.50) * g_frameBufferWidthRealRecip;
					const double pixelCenterX = shaderFrameBuffer.xPercent * g_frameBufferWidthReal;

					double pixelCoverageDot0X, pixelCoverageDot1X, pixelCoverageDot2X;
					GetScreenSpacePointHalfSpaceComponents(pixelCenterX, screenSpace0X, screenSpace1X, screenSpace2X, screenSpace01PerpX,
						screenSpace12PerpX, screenSpace20PerpX, &pixelCoverageDot0X, &pixelCoverageDot1X, &pixelCoverageDot2X);

					const double pixelCenterDot0 = pixelCoverageDot0X + pixelCoverageDot0Y;
					const double pixelCenterDot1 = pixelCoverageDot1X + pixelCoverageDot1Y;
					const double pixelCenterDot2 = pixelCoverageDot2X + pixelCoverageDot2Y;
					const bool isPixelCenterIn0 = pixelCenterDot0 >= 0.0;
					const bool isPixelCenterIn1 = pixelCenterDot1 >= 0.0;
					const bool isPixelCenterIn2 = pixelCenterDot2 >= 0.0;
					const bool pixelCenterHasCoverage = isPixelCenterIn0 && isPixelCenterIn1 && isPixelCenterIn2;
					
					totalCoverageTests++;

					if (!pixelCenterHasCoverage)
					{
						continue;
					}

					const double screenSpace0CurrentX = pixelCenterX - screenSpace0X;
					const double barycentricDot20X = screenSpace0CurrentX * screenSpace01X;
					const double barycentricDot21X = screenSpace0CurrentX * screenSpace02X;
					const double barycentricDot20 = barycentricDot20X + barycentricDot20Y;
					const double barycentricDot21 = barycentricDot21X + barycentricDot21Y;
					const double vNumerator = (barycentricDot11 * barycentricDot20) - (barycentricDot01 * barycentricDot21);
					const double wNumerator = (barycentricDot00 * barycentricDot21) - (barycentricDot01 * barycentricDot20);
					const double v = vNumerator * barycentricDenominatorRecip;
					const double w = wNumerator * barycentricDenominatorRecip;
					const double u = 1.0 - v - w;

					PixelShaderPerspectiveCorrection shaderPerspective;
					shaderPerspective.ndcZDepth = (ndc0Z * u) + (ndc1Z * v) + (ndc2Z * w);

					bool passesDepthTest = true;
					if constexpr (enableDepthRead)
					{
						passesDepthTest = shaderPerspective.ndcZDepth < g_depthBuffer[shaderFrameBuffer.pixelIndex];
						totalDepthTests++;
					}

					if (!passesDepthTest)
					{
						continue;
					}

					const double shaderClipSpacePointX = (ndc0X * u) + (ndc1X * v) + (ndc2X * w);
					const double shaderClipSpacePointY = (ndc0Y * u) + (ndc1Y * v) + (ndc2Y * w);
					const double shaderClipSpacePointZ = (ndc0Z * u) + (ndc1Z * v) + (ndc2Z * w);
					const double shaderClipSpacePointW = (clip0WRecip * u) + (clip1WRecip * v) + (clip2WRecip * w);
					const double shaderClipSpacePointWRecip = 1.0 / shaderClipSpacePointW;
					const double shaderHomogeneousSpacePointX = shaderClipSpacePointX * shaderClipSpacePointWRecip;
					const double shaderHomogeneousSpacePointY = shaderClipSpacePointY * shaderClipSpacePointWRecip;
					const double shaderHomogeneousSpacePointZ = shaderClipSpacePointZ * shaderClipSpacePointWRecip;
					const double shaderHomogeneousSpacePointW = shaderClipSpacePointWRecip;

					// Apply homogeneous-to-camera space transform.
					double shaderCameraSpacePointX = 0.0;
					double shaderCameraSpacePointY = 0.0;
					double shaderCameraSpacePointZ = 0.0;
					double shaderCameraSpacePointW = 0.0;
					Matrix4_MultiplyVectorN<1>(
						g_invProjMatrixXX, g_invProjMatrixXY, g_invProjMatrixXZ, g_invProjMatrixXW,
						g_invProjMatrixYX, g_invProjMatrixYY, g_invProjMatrixYZ, g_invProjMatrixYW,
						g_invProjMatrixZX, g_invProjMatrixZY, g_invProjMatrixZZ, g_invProjMatrixZW,
						g_invProjMatrixWX, g_invProjMatrixWY, g_invProjMatrixWZ, g_invProjMatrixWW,
						&shaderHomogeneousSpacePointX, &shaderHomogeneousSpacePointY, &shaderHomogeneousSpacePointZ, &shaderHomogeneousSpacePointW,
						&shaderCameraSpacePointX, &shaderCameraSpacePointY, &shaderCameraSpacePointZ, &shaderCameraSpacePointW);

					// Apply camera-to-world space transform.
					double shaderWorldSpacePointX = 0.0;
					double shaderWorldSpacePointY = 0.0;
					double shaderWorldSpacePointZ = 0.0;
					Matrix4_MultiplyVectorIgnoreW_N<1>(
						g_invViewMatrixXX, g_invViewMatrixXY, g_invViewMatrixXZ,
						g_invViewMatrixYX, g_invViewMatrixYY, g_invViewMatrixYZ,
						g_invViewMatrixZX, g_invViewMatrixZY, g_invViewMatrixZZ,
						g_invViewMatrixWX, g_invViewMatrixWY, g_invViewMatrixWZ,
						&shaderCameraSpacePointX, &shaderCameraSpacePointY, &shaderCameraSpacePointZ, &shaderCameraSpacePointW,
						&shaderWorldSpacePointX, &shaderWorldSpacePointY, &shaderWorldSpacePointZ);

					shaderPerspective.texelPercentX = ((uv0XDivW * u) + (uv1XDivW * v) + (uv2XDivW * w)) * shaderClipSpacePointWRecip;
					shaderPerspective.texelPercentY = ((uv0YDivW * u) + (uv1YDivW * v) + (uv2YDivW * w)) * shaderClipSpacePointWRecip;

					double lightIntensitySum = 0.0;
					if constexpr (requiresPerPixelLightIntensity)
					{
						lightIntensitySum = g_ambientPercent;
						for (int lightIndex = 0; lightIndex < lightCount; lightIndex++)
						{
							const SoftwareRenderer::Light &light = *lights[lightIndex];
							double lightIntensity;
							GetWorldSpaceLightIntensityValue(shaderWorldSpacePointX, shaderWorldSpacePointY, shaderWorldSpacePointZ, light, &lightIntensity);
							lightIntensitySum += lightIntensity;

							if (lightIntensitySum >= 1.0)
							{
								lightIntensitySum = 1.0;
								break;
							}
						}
					}
					else if (requiresPerMeshLightIntensity)
					{
						lightIntensitySum = meshLightPercent;
					}

					const double lightLevelReal = lightIntensitySum * shaderLighting.lightLevelCountReal;
					shaderLighting.lightLevel = shaderLighting.lastLightLevel - std::clamp(static_cast<int>(lightLevelReal), 0, shaderLighting.lastLightLevel);

					if constexpr (requiresPerPixelLightIntensity)
					{
						bool shouldDither;
						GetScreenSpaceDitherValue<ditheringMode>(lightLevelReal, lightIntensitySum, shaderFrameBuffer.pixelIndex, &shouldDither);

						if (shouldDither)
						{
							shaderLighting.lightLevel = std::min(shaderLighting.lightLevel + 1, shaderLighting.lastLightLevel);
						}
					}

					if constexpr (requiresHorizonMirror)
					{
						// @todo: support camera roll
						const double reflectedScreenSpacePointX = pixelCenterX;
						const double reflectedScreenSpacePointY = shaderHorizonMirror.horizonScreenSpacePointY + (shaderHorizonMirror.horizonScreenSpacePointY - pixelCenterY);

						const int reflectedPixelX = static_cast<int>(reflectedScreenSpacePointX);
						const int reflectedPixelY = static_cast<int>(reflectedScreenSpacePointY);
						shaderHorizonMirror.isReflectedPixelInFrameBuffer =
							(reflectedPixelX >= 0) && (reflectedPixelX < g_frameBufferWidth) &&
							(reflectedPixelY >= 0) && (reflectedPixelY < g_frameBufferHeight);
						shaderHorizonMirror.reflectedPixelIndex = reflectedPixelX + (reflectedPixelY * g_frameBufferWidth);
					}

					if constexpr (pixelShaderType == PixelShaderType::Opaque)
					{
						PixelShader_Opaque<enableDepthWrite>(shaderPerspective, shaderTexture0, shaderLighting, shaderFrameBuffer);
					}
					else if (pixelShaderType == PixelShaderType::OpaqueWithAlphaTestLayer)
					{
						PixelShader_OpaqueWithAlphaTestLayer<enableDepthWrite>(shaderPerspective, shaderTexture0, shaderTexture1, shaderLighting, shaderFrameBuffer);
					}
					else if (pixelShaderType == PixelShaderType::OpaqueScreenSpaceAnimation)
					{
						PixelShader_OpaqueScreenSpaceAnimation<enableDepthWrite>(shaderPerspective, shaderTexture0, shaderLighting, shaderUniforms, shaderFrameBuffer);
					}
					else if (pixelShaderType == PixelShaderType::OpaqueScreenSpaceAnimationWithAlphaTestLayer)
					{
						PixelShader_OpaqueScreenSpaceAnimationWithAlphaTestLayer<enableDepthWrite>(shaderPerspective, shaderTexture0, shaderTexture1, shaderLighting, shaderUniforms, shaderFrameBuffer);
					}
					else if (pixelShaderType == PixelShaderType::AlphaTested)
					{
						PixelShader_AlphaTested<enableDepthWrite>(shaderPerspective, shaderTexture0, shaderLighting, shaderFrameBuffer);
					}
					else if (pixelShaderType == PixelShaderType::AlphaTestedWithVariableTexCoordUMin)
					{
						PixelShader_AlphaTestedWithVariableTexCoordUMin<enableDepthWrite>(shaderPerspective, shaderTexture0, pixelShaderParam0, shaderLighting, shaderFrameBuffer);
					}
					else if (pixelShaderType == PixelShaderType::AlphaTestedWithVariableTexCoordVMin)
					{
						PixelShader_AlphaTestedWithVariableTexCoordVMin<enableDepthWrite>(shaderPerspective, shaderTexture0, pixelShaderParam0, shaderLighting, shaderFrameBuffer);
					}
					else if (pixelShaderType == PixelShaderType::AlphaTestedWithPaletteIndexLookup)
					{
						PixelShader_AlphaTestedWithPaletteIndexLookup<enableDepthWrite>(shaderPerspective, shaderTexture0, shaderTexture1, shaderLighting, shaderFrameBuffer);
					}
					else if (pixelShaderType == PixelShaderType::AlphaTestedWithLightLevelColor)
					{
						PixelShader_AlphaTestedWithLightLevelColor<enableDepthWrite>(shaderPerspective, shaderTexture0, shaderLighting, shaderFrameBuffer);
					}
					else if (pixelShaderType == PixelShaderType::AlphaTestedWithLightLevelOpacity)
					{
						PixelShader_AlphaTestedWithLightLevelOpacity<enableDepthWrite>(shaderPerspective, shaderTexture0, shaderLighting, shaderFrameBuffer);
					}
					else if (pixelShaderType == PixelShaderType::AlphaTestedWithPreviousBrightnessLimit)
					{
						PixelShader_AlphaTestedWithPreviousBrightnessLimit<enableDepthWrite>(shaderPerspective, shaderTexture0, shaderFrameBuffer);
					}
					else if (pixelShaderType == PixelShaderType::AlphaTestedWithHorizonMirror)
					{
						PixelShader_AlphaTestedWithHorizonMirror<enableDepthWrite>(shaderPerspective, shaderTexture0, shaderHorizonMirror, shaderLighting, shaderFrameBuffer);
					}

					// Write pixel shader result to final output buffer. This only results in overdraw for ghosts.
					const uint8_t writtenPaletteIndex = g_paletteIndexBuffer[shaderFrameBuffer.pixelIndex];
					g_colorBuffer[shaderFrameBuffer.pixelIndex] = shaderFrameBuffer.palette.colors[writtenPaletteIndex];
					totalColorWrites++;
				}
			}
		}

		g_totalCoverageTests += totalCoverageTests;
		g_totalDepthTests += totalDepthTests;
		g_totalColorWrites += totalColorWrites;
	}

	template<RenderLightingType lightingType, PixelShaderType pixelShaderType, bool enableDepthRead, bool enableDepthWrite>
	void RasterizeMeshDispatchDitheringMode(const DrawCallCache &drawCallCache, const RasterizerInputCache &rasterizerInputCache, const RasterizerBin &bin,
		const RasterizerBinEntry &binEntry, int binX, int binY, int binIndex)
	{
		switch (g_ditheringMode)
		{
		case DitheringMode::None:
			RasterizeMeshInternal<lightingType, pixelShaderType, enableDepthRead, enableDepthWrite, DitheringMode::None>(drawCallCache, rasterizerInputCache, bin, binEntry, binX, binY, binIndex);
			break;
		case DitheringMode::Classic:
			RasterizeMeshInternal<lightingType, pixelShaderType, enableDepthRead, enableDepthWrite, DitheringMode::Classic>(drawCallCache, rasterizerInputCache, bin, binEntry, binX, binY, binIndex);
			break;
		case DitheringMode::Modern:
			RasterizeMeshInternal<lightingType, pixelShaderType, enableDepthRead, enableDepthWrite, DitheringMode::Modern>(drawCallCache, rasterizerInputCache, bin, binEntry, binX, binY, binIndex);
			break;
		}
	}

	template<RenderLightingType lightingType, PixelShaderType pixelShaderType>
	void RasterizeMeshDispatchDepthToggles(const DrawCallCache &drawCallCache, const RasterizerInputCache &rasterizerInputCache, const RasterizerBin &bin,
		const RasterizerBinEntry &binEntry, int binX, int binY, int binIndex)
	{
		const bool enableDepthRead = drawCallCache.enableDepthRead;
		const bool enableDepthWrite = drawCallCache.enableDepthWrite;

		if (enableDepthRead)
		{
			if (enableDepthWrite)
			{
				RasterizeMeshDispatchDitheringMode<lightingType, pixelShaderType, true, true>(drawCallCache, rasterizerInputCache, bin, binEntry, binX, binY, binIndex);
			}
			else
			{
				RasterizeMeshDispatchDitheringMode<lightingType, pixelShaderType, true, false>(drawCallCache, rasterizerInputCache, bin, binEntry, binX, binY, binIndex);
			}
		}
		else
		{
			if (enableDepthWrite)
			{
				RasterizeMeshDispatchDitheringMode<lightingType, pixelShaderType, false, true>(drawCallCache, rasterizerInputCache, bin, binEntry, binX, binY, binIndex);
			}
			else
			{
				RasterizeMeshDispatchDitheringMode<lightingType, pixelShaderType, false, false>(drawCallCache, rasterizerInputCache, bin, binEntry, binX, binY, binIndex);
			}
		}
	}

	template<RenderLightingType lightingType>
	void RasterizeMeshDispatchPixelShaderType(const DrawCallCache &drawCallCache, const RasterizerInputCache &rasterizerInputCache, const RasterizerBin &bin,
		const RasterizerBinEntry &binEntry, int binX, int binY, int binIndex)
	{
		static_assert(PixelShaderType::AlphaTestedWithHorizonMirror == PIXEL_SHADER_TYPE_MAX);
		const PixelShaderType pixelShaderType = drawCallCache.pixelShaderType;

		switch (pixelShaderType)
		{
		case PixelShaderType::Opaque:
			RasterizeMeshDispatchDepthToggles<lightingType, PixelShaderType::Opaque>(drawCallCache, rasterizerInputCache, bin, binEntry, binX, binY, binIndex);
			break;
		case PixelShaderType::OpaqueWithAlphaTestLayer:
			RasterizeMeshDispatchDepthToggles<lightingType, PixelShaderType::OpaqueWithAlphaTestLayer>(drawCallCache, rasterizerInputCache, bin, binEntry, binX, binY, binIndex);
			break;
		case PixelShaderType::OpaqueScreenSpaceAnimation:
			RasterizeMeshDispatchDepthToggles<lightingType, PixelShaderType::OpaqueScreenSpaceAnimation>(drawCallCache, rasterizerInputCache, bin, binEntry, binX, binY, binIndex);
			break;
		case PixelShaderType::OpaqueScreenSpaceAnimationWithAlphaTestLayer:
			RasterizeMeshDispatchDepthToggles<lightingType, PixelShaderType::OpaqueScreenSpaceAnimationWithAlphaTestLayer>(drawCallCache, rasterizerInputCache, bin, binEntry, binX, binY, binIndex);
			break;
		case PixelShaderType::AlphaTested:
			RasterizeMeshDispatchDepthToggles<lightingType, PixelShaderType::AlphaTested>(drawCallCache, rasterizerInputCache, bin, binEntry, binX, binY, binIndex);
			break;
		case PixelShaderType::AlphaTestedWithVariableTexCoordUMin:
			RasterizeMeshDispatchDepthToggles<lightingType, PixelShaderType::AlphaTestedWithVariableTexCoordUMin>(drawCallCache, rasterizerInputCache, bin, binEntry, binX, binY, binIndex);
			break;
		case PixelShaderType::AlphaTestedWithVariableTexCoordVMin:
			RasterizeMeshDispatchDepthToggles<lightingType, PixelShaderType::AlphaTestedWithVariableTexCoordVMin>(drawCallCache, rasterizerInputCache, bin, binEntry, binX, binY, binIndex);
			break;
		case PixelShaderType::AlphaTestedWithPaletteIndexLookup:
			RasterizeMeshDispatchDepthToggles<lightingType, PixelShaderType::AlphaTestedWithPaletteIndexLookup>(drawCallCache, rasterizerInputCache, bin, binEntry, binX, binY, binIndex);
			break;
		case PixelShaderType::AlphaTestedWithLightLevelColor:
			RasterizeMeshDispatchDepthToggles<lightingType, PixelShaderType::AlphaTestedWithLightLevelColor>(drawCallCache, rasterizerInputCache, bin, binEntry, binX, binY, binIndex);
			break;
		case PixelShaderType::AlphaTestedWithLightLevelOpacity:
			RasterizeMeshDispatchDepthToggles<lightingType, PixelShaderType::AlphaTestedWithLightLevelOpacity>(drawCallCache, rasterizerInputCache, bin, binEntry, binX, binY, binIndex);
			break;
		case PixelShaderType::AlphaTestedWithPreviousBrightnessLimit:
			RasterizeMeshDispatchDepthToggles<lightingType, PixelShaderType::AlphaTestedWithPreviousBrightnessLimit>(drawCallCache, rasterizerInputCache, bin, binEntry, binX, binY, binIndex);
			break;
		case PixelShaderType::AlphaTestedWithHorizonMirror:
			RasterizeMeshDispatchDepthToggles<lightingType, PixelShaderType::AlphaTestedWithHorizonMirror>(drawCallCache, rasterizerInputCache, bin, binEntry, binX, binY, binIndex);
			break;
		}
	}

	// Decides which optimized rasterizer variant to use based on the parameters.
	void RasterizeMesh(const DrawCallCache &drawCallCache, const RasterizerInputCache &rasterizerInputCache, const RasterizerBin &bin,
		const RasterizerBinEntry &binEntry, int binX, int binY, int binIndex)
	{
		static_assert(RenderLightingType::PerPixel == RENDER_LIGHTING_TYPE_MAX);
		const RenderLightingType lightingType = drawCallCache.lightingType;

		switch (lightingType)
		{
		case RenderLightingType::PerMesh:
			RasterizeMeshDispatchPixelShaderType<RenderLightingType::PerMesh>(drawCallCache, rasterizerInputCache, bin, binEntry, binX, binY, binIndex);
			break;
		case RenderLightingType::PerPixel:
			RasterizeMeshDispatchPixelShaderType<RenderLightingType::PerPixel>(drawCallCache, rasterizerInputCache, bin, binEntry, binX, binY, binIndex);
			break;
		}
	}
}

// Multi-threading utils.
namespace
{
	struct Worker
	{
		std::thread thread;
		
		DrawCallCache drawCallCaches[MAX_WORKER_DRAW_CALLS_PER_LOOP];
		TransformCache transformCaches[MAX_WORKER_DRAW_CALLS_PER_LOOP];
		int drawCallStartIndex, drawCallCount;

		VertexShaderInputCache vertexShaderInputCache;
		VertexShaderOutputCache vertexShaderOutputCache;
		ClippingOutputCache clippingOutputCache;
		RasterizerInputCache rasterizerInputCache;
		std::vector<RasterizerWorkItem> rasterizerWorkItems;
		bool isReadyToStartFrame, shouldExit, shouldWorkOnDrawCalls, isFinishedWithDrawCalls, shouldWorkOnRasterizing, isFinishedRasterizing;
	};

	Buffer<Worker> g_workers;
	std::mutex g_mutex;
	std::condition_variable g_workerCondVar, g_directorCondVar;

	void WorkerFunc(int workerIndex)
	{
		Worker &worker = g_workers.get(workerIndex);
		std::unique_lock<std::mutex> workerLock(g_mutex);

		while (true)
		{
			worker.isReadyToStartFrame = true;
			g_directorCondVar.notify_one();
			g_workerCondVar.wait(workerLock, [&worker]() { return worker.shouldExit || worker.shouldWorkOnDrawCalls; });
			workerLock.unlock();

			if (worker.shouldExit)
			{
				break;
			}

			for (int drawCallIndex = 0; drawCallIndex < worker.drawCallCount; drawCallIndex++)
			{
				DebugAssertIndex(worker.drawCallCaches, drawCallIndex);
				const DrawCallCache &drawCallCache = worker.drawCallCaches[drawCallIndex];
				TransformCache &transformCache = worker.transformCaches[drawCallIndex];
				VertexShaderInputCache &vertexShaderInputCache = worker.vertexShaderInputCache;
				VertexShaderOutputCache &vertexShaderOutputCache = worker.vertexShaderOutputCache;
				ClippingOutputCache &clippingOutputCache = worker.clippingOutputCache;
				RasterizerInputCache &rasterizerInputCache = worker.rasterizerInputCache;

				ProcessMeshBufferLookups(drawCallCache, vertexShaderInputCache);
				CalculateVertexShaderTransforms(transformCache);
				ProcessVertexShaders(drawCallCache.vertexShaderType, transformCache, vertexShaderInputCache, vertexShaderOutputCache);
				ProcessClipping(drawCallCache, vertexShaderOutputCache, clippingOutputCache);
				ProcessClipSpaceTrianglesForBinning(drawCallIndex, clippingOutputCache, rasterizerInputCache);
			}

			workerLock.lock();
			worker.isFinishedWithDrawCalls = true;
			g_directorCondVar.notify_one();
			g_workerCondVar.wait(workerLock, [&worker]() { return worker.shouldWorkOnRasterizing; });
			workerLock.unlock();

			// Use the geometry processing results of all workers to rasterize this worker's bins. The order of workers is assumed to be
			// the same that draw calls were originally processed, otherwise triangles in each bin would be rasterized in the wrong order.
			for (const RasterizerWorkItem &workItem : worker.rasterizerWorkItems)
			{
				const int binX = workItem.binX;
				const int binY = workItem.binY;
				for (const Worker &geometryWorker : g_workers)
				{
					if (geometryWorker.drawCallCount > 0)
					{
						const RasterizerBin &geometryWorkerBin = geometryWorker.rasterizerInputCache.bins.get(binX, binY);
						for (int entryIndex = 0; entryIndex < geometryWorkerBin.entryCount; entryIndex++)
						{
							const RasterizerBinEntry &binEntry = geometryWorkerBin.entries[entryIndex];
							const int workerDrawCallIndex = binEntry.workerDrawCallIndex;
							DebugAssertIndex(geometryWorker.drawCallCaches, workerDrawCallIndex);
							const DrawCallCache &drawCallCache = geometryWorker.drawCallCaches[workerDrawCallIndex];
							const RasterizerInputCache &rasterizerInputCache = geometryWorker.rasterizerInputCache;
							RasterizeMesh(drawCallCache, rasterizerInputCache, geometryWorkerBin, binEntry, binX, binY, workItem.binIndex);
						}
					}
				}
			}

			workerLock.lock();
			worker.isFinishedRasterizing = true;
		}
	}

	void SignalWorkersToExitAndJoin()
	{
		std::unique_lock<std::mutex> lock(g_mutex);
		for (Worker &worker : g_workers)
		{
			worker.shouldExit = true;
		}

		g_workerCondVar.notify_all();
		lock.unlock();

		for (Worker &worker : g_workers)
		{
			worker.thread.join();
		}
	}

	void InitializeWorkers(int workerCount, int frameBufferWidth, int frameBufferHeight)
	{
		if (g_workers.getCount() != workerCount)
		{
			SignalWorkersToExitAndJoin();

			g_workers.init(workerCount);
			for (int workerIndex = 0; workerIndex < workerCount; workerIndex++)
			{
				Worker &worker = g_workers[workerIndex];
				worker.drawCallStartIndex = -1;
				worker.drawCallCount = 0;
				worker.rasterizerInputCache.createBins(frameBufferWidth, frameBufferHeight);
				worker.isReadyToStartFrame = false;
				worker.shouldExit = false;
				worker.shouldWorkOnDrawCalls = false;
				worker.isFinishedWithDrawCalls = false;
				worker.shouldWorkOnRasterizing = false;
				worker.isFinishedRasterizing = false;
				worker.thread = std::thread(WorkerFunc, workerIndex);
			}
		}

		for (Worker &worker : g_workers)
		{
			worker.rasterizerWorkItems.clear();
		}

		const Worker &firstWorker = g_workers.get(0);
		const int binCountX = firstWorker.rasterizerInputCache.binCountX;
		const int binCountY = firstWorker.rasterizerInputCache.binCountY;

		// Split up rasterizer bins across workers.
		int curWorkerIndex = 0;
		for (int binY = 0; binY < binCountY; binY++)
		{
			for (int binX = 0; binX < binCountX; binX++)
			{
				const int binIndex = binX + (binY * binCountX);
				Worker &worker = g_workers[curWorkerIndex];
				worker.rasterizerWorkItems.emplace_back(binX, binY, binIndex);
				curWorkerIndex = (curWorkerIndex + 1) % workerCount;
			}
		}
	}

	void PopulateWorkerDrawCallWorkloads(int workerCount, int startDrawCallIndex, int drawCallCount)
	{
		const int baseDrawCallsPerWorker = drawCallCount / workerCount;
		const int workersWithExtraDrawCall = drawCallCount % workerCount;

		int workerStartDrawCallIndex = startDrawCallIndex;
		for (int i = 0; i < workerCount; i++)
		{
			Worker &worker = g_workers[i];
			worker.drawCallStartIndex = workerStartDrawCallIndex;
			worker.drawCallCount = baseDrawCallsPerWorker;

			if (i < workersWithExtraDrawCall)
			{
				worker.drawCallCount++;
			}

			workerStartDrawCallIndex += worker.drawCallCount;
		}

		DebugAssert((workerStartDrawCallIndex - startDrawCallIndex) == drawCallCount);
	}

	void ShutdownWorkers()
	{
		SignalWorkersToExitAndJoin();
		g_workers.clear();
	}
}

SoftwareRenderer::ObjectTexture::ObjectTexture()
{
	this->texels8Bit = nullptr;
	this->texels32Bit = nullptr;
	this->width = 0;
	this->height = 0;
	this->widthReal = 0.0;
	this->heightReal = 0.0;
	this->texelCount = 0;
	this->bytesPerTexel = 0;
}

void SoftwareRenderer::ObjectTexture::init(int width, int height, int bytesPerTexel)
{
	DebugAssert(width > 0);
	DebugAssert(height > 0);
	DebugAssert(bytesPerTexel > 0);

	this->texelCount = width * height;
	this->texels.init(this->texelCount * bytesPerTexel);
	this->texels.fill(static_cast<std::byte>(0));

	switch (bytesPerTexel)
	{
	case 1:
		this->texels8Bit = reinterpret_cast<const uint8_t*>(this->texels.begin());
		break;
	case 4:
		this->texels32Bit = reinterpret_cast<const uint32_t*>(this->texels.begin());
		break;
	default:
		DebugNotImplementedMsg(std::to_string(bytesPerTexel));
		break;
	}

	this->width = width;
	this->height = height;
	this->widthReal = static_cast<double>(width);
	this->heightReal = static_cast<double>(height);
	this->bytesPerTexel = bytesPerTexel;
}

void SoftwareRenderer::ObjectTexture::clear()
{
	this->texels.clear();
}

void SoftwareRenderer::VertexBuffer::init(int vertexCount, int componentsPerVertex)
{
	const int valueCount = vertexCount * componentsPerVertex;
	this->vertices.init(valueCount);
}

void SoftwareRenderer::AttributeBuffer::init(int vertexCount, int componentsPerVertex)
{
	const int valueCount = vertexCount * componentsPerVertex;
	this->attributes.init(valueCount);
}

void SoftwareRenderer::IndexBuffer::init(int indexCount)
{
	DebugAssertMsg((indexCount % 3) == 0, "Expected index buffer to have multiple of 3 indices (has " + std::to_string(indexCount) + ").");
	this->indices.init(indexCount);
	this->triangleCount = indexCount / 3;
}

SoftwareRenderer::Light::Light()
{
	this->worldPointX = 0.0;
	this->worldPointY = 0.0;
	this->worldPointZ = 0.0;
	this->startRadius = 0.0;
	this->startRadiusSqr = 0.0;
	this->endRadius = 0.0;
	this->endRadiusSqr = 0.0;
	this->startEndRadiusDiff = 0.0;
	this->startEndRadiusDiffRecip = 0.0;
}

void SoftwareRenderer::Light::init(const Double3 &worldPoint, double startRadius, double endRadius)
{
	this->worldPointX = worldPoint.x;
	this->worldPointY = worldPoint.y;
	this->worldPointZ = worldPoint.z;
	this->startRadius = startRadius;
	this->startRadiusSqr = startRadius * startRadius;
	this->endRadius = endRadius;
	this->endRadiusSqr = endRadius * endRadius;
	this->startEndRadiusDiff = endRadius - startRadius;
	this->startEndRadiusDiffRecip = 1.0 / this->startEndRadiusDiff;
}

SoftwareRenderer::SoftwareRenderer()
{
	this->ditheringMode = static_cast<DitheringMode>(-1);
}

SoftwareRenderer::~SoftwareRenderer()
{

}

void SoftwareRenderer::init(const RenderInitSettings &settings)
{
	const int frameBufferWidth = settings.width;
	const int frameBufferHeight = settings.height;
	this->paletteIndexBuffer.init(frameBufferWidth, frameBufferHeight);
	this->depthBuffer.init(frameBufferWidth, frameBufferHeight);

	CreateDitherBuffer(this->ditherBuffer, frameBufferWidth, frameBufferHeight, settings.ditheringMode);
	this->ditheringMode = settings.ditheringMode;

	const int workerCount = RendererUtils::getRenderThreadsFromMode(settings.renderThreadsMode);
	InitializeWorkers(workerCount, frameBufferWidth, frameBufferHeight);
}

void SoftwareRenderer::shutdown()
{
	this->paletteIndexBuffer.clear();
	this->depthBuffer.clear();
	this->ditherBuffer.clear();
	this->ditheringMode = static_cast<DitheringMode>(-1);
	this->vertexBuffers.clear();
	this->attributeBuffers.clear();
	this->indexBuffers.clear();
	this->uniformBuffers.clear();
	this->objectTextures.clear();
	this->lights.clear();
	ShutdownWorkers();
}

bool SoftwareRenderer::isInited() const
{
	return true;
}

void SoftwareRenderer::resize(int width, int height)
{
	this->paletteIndexBuffer.init(width, height);
	this->paletteIndexBuffer.fill(0);

	this->depthBuffer.init(width, height);
	this->depthBuffer.fill(std::numeric_limits<double>::infinity());

	CreateDitherBuffer(this->ditherBuffer, width, height, this->ditheringMode);

	for (Worker &worker : g_workers)
	{
		worker.rasterizerInputCache.createBins(width, height);
	}
}

bool SoftwareRenderer::tryCreateVertexBuffer(int vertexCount, int componentsPerVertex, VertexBufferID *outID)
{
	DebugAssert(vertexCount > 0);
	DebugAssert(componentsPerVertex >= 2);

	if (!this->vertexBuffers.tryAlloc(outID))
	{
		DebugLogError("Couldn't allocate vertex buffer ID.");
		return false;
	}

	VertexBuffer &buffer = this->vertexBuffers.get(*outID);
	buffer.init(vertexCount, componentsPerVertex);
	return true;
}

bool SoftwareRenderer::tryCreateAttributeBuffer(int vertexCount, int componentsPerVertex, AttributeBufferID *outID)
{
	DebugAssert(vertexCount > 0);
	DebugAssert(componentsPerVertex >= 2);

	if (!this->attributeBuffers.tryAlloc(outID))
	{
		DebugLogError("Couldn't allocate attribute buffer ID.");
		return false;
	}

	AttributeBuffer &buffer = this->attributeBuffers.get(*outID);
	buffer.init(vertexCount, componentsPerVertex);
	return true;
}

bool SoftwareRenderer::tryCreateIndexBuffer(int indexCount, IndexBufferID *outID)
{
	DebugAssert(indexCount > 0);
	DebugAssert((indexCount % 3) == 0);

	if (!this->indexBuffers.tryAlloc(outID))
	{
		DebugLogError("Couldn't allocate index buffer ID.");
		return false;
	}

	IndexBuffer &buffer = this->indexBuffers.get(*outID);
	buffer.init(indexCount);
	return true;
}

void SoftwareRenderer::populateVertexBuffer(VertexBufferID id, BufferView<const double> vertices)
{
	VertexBuffer &buffer = this->vertexBuffers.get(id);
	const int srcCount = vertices.getCount();
	const int dstCount = buffer.vertices.getCount();
	if (srcCount != dstCount)
	{
		DebugLogError("Mismatched vertex buffer sizes for ID " + std::to_string(id) + ": " +
			std::to_string(srcCount) + " != " + std::to_string(dstCount));
		return;
	}

	const auto srcBegin = vertices.begin();
	const auto srcEnd = srcBegin + srcCount;
	std::copy(srcBegin, srcEnd, buffer.vertices.begin());
}

void SoftwareRenderer::populateAttributeBuffer(AttributeBufferID id, BufferView<const double> attributes)
{
	AttributeBuffer &buffer = this->attributeBuffers.get(id);
	const int srcCount = attributes.getCount();
	const int dstCount = buffer.attributes.getCount();
	if (srcCount != dstCount)
	{
		DebugLogError("Mismatched attribute buffer sizes for ID " + std::to_string(id) + ": " +
			std::to_string(srcCount) + " != " + std::to_string(dstCount));
		return;
	}

	const auto srcBegin = attributes.begin();
	const auto srcEnd = srcBegin + srcCount;
	std::copy(srcBegin, srcEnd, buffer.attributes.begin());
}

void SoftwareRenderer::populateIndexBuffer(IndexBufferID id, BufferView<const int32_t> indices)
{
	IndexBuffer &buffer = this->indexBuffers.get(id);
	const int srcCount = indices.getCount();
	const int dstCount = buffer.indices.getCount();
	if (srcCount != dstCount)
	{
		DebugLogError("Mismatched index buffer sizes for ID " + std::to_string(id) + ": " +
			std::to_string(srcCount) + " != " + std::to_string(dstCount));
		return;
	}

	const auto srcBegin = indices.begin();
	const auto srcEnd = srcBegin + srcCount;
	std::copy(srcBegin, srcEnd, buffer.indices.begin());
}

void SoftwareRenderer::freeVertexBuffer(VertexBufferID id)
{
	this->vertexBuffers.free(id);
}

void SoftwareRenderer::freeAttributeBuffer(AttributeBufferID id)
{
	this->attributeBuffers.free(id);
}

void SoftwareRenderer::freeIndexBuffer(IndexBufferID id)
{
	this->indexBuffers.free(id);
}

bool SoftwareRenderer::tryCreateObjectTexture(int width, int height, int bytesPerTexel, ObjectTextureID *outID)
{
	if (!this->objectTextures.tryAlloc(outID))
	{
		DebugLogError("Couldn't allocate object texture ID.");
		return false;
	}

	ObjectTexture &texture = this->objectTextures.get(*outID);
	texture.init(width, height, bytesPerTexel);
	return true;
}

bool SoftwareRenderer::tryCreateObjectTexture(const TextureBuilder &textureBuilder, ObjectTextureID *outID)
{
	const int width = textureBuilder.getWidth();
	const int height = textureBuilder.getHeight();
	const int bytesPerTexel = textureBuilder.getBytesPerTexel();
	if (!this->tryCreateObjectTexture(width, height, bytesPerTexel, outID))
	{
		DebugLogWarning("Couldn't create " + std::to_string(width) + "x" + std::to_string(height) + " object texture.");
		return false;
	}

	const TextureBuilderType textureBuilderType = textureBuilder.type;
	ObjectTexture &texture = this->objectTextures.get(*outID);
	if (textureBuilderType == TextureBuilderType::Paletted)
	{
		const TextureBuilderPalettedTexture &palettedTexture = textureBuilder.paletteTexture;
		const Buffer2D<uint8_t> &srcTexels = palettedTexture.texels;
		uint8_t *dstTexels = reinterpret_cast<uint8_t*>(texture.texels.begin());
		std::copy(srcTexels.begin(), srcTexels.end(), dstTexels);
	}
	else if (textureBuilderType == TextureBuilderType::TrueColor)
	{
		const TextureBuilderTrueColorTexture &trueColorTexture = textureBuilder.trueColorTexture;
		const Buffer2D<uint32_t> &srcTexels = trueColorTexture.texels;
		uint32_t *dstTexels = reinterpret_cast<uint32_t*>(texture.texels.begin());
		std::copy(srcTexels.begin(), srcTexels.end(), dstTexels);
	}
	else
	{
		DebugUnhandledReturnMsg(bool, std::to_string(static_cast<int>(textureBuilderType)));
	}

	return true;
}

LockedTexture SoftwareRenderer::lockObjectTexture(ObjectTextureID id)
{
	ObjectTexture &texture = this->objectTextures.get(id);
	return LockedTexture(texture.texels.begin(), texture.bytesPerTexel);
}

void SoftwareRenderer::unlockObjectTexture(ObjectTextureID id)
{
	// Do nothing; any writes are already in RAM.
	static_cast<void>(id);
}

void SoftwareRenderer::freeObjectTexture(ObjectTextureID id)
{
	this->objectTextures.free(id);
}

std::optional<Int2> SoftwareRenderer::tryGetObjectTextureDims(ObjectTextureID id) const
{
	const ObjectTexture &texture = this->objectTextures.get(id);
	return Int2(texture.width, texture.height);
}

bool SoftwareRenderer::tryCreateUniformBuffer(int elementCount, size_t sizeOfElement, size_t alignmentOfElement, UniformBufferID *outID)
{
	DebugAssert(elementCount >= 0);
	DebugAssert(sizeOfElement > 0);
	DebugAssert(alignmentOfElement > 0);

	if (!this->uniformBuffers.tryAlloc(outID))
	{
		DebugLogError("Couldn't allocate uniform buffer ID.");
		return false;
	}

	UniformBuffer &buffer = this->uniformBuffers.get(*outID);
	buffer.init(elementCount, sizeOfElement, alignmentOfElement);
	return true;
}

void SoftwareRenderer::populateUniformBuffer(UniformBufferID id, BufferView<const std::byte> data)
{
	UniformBuffer &buffer = this->uniformBuffers.get(id);
	const int srcCount = data.getCount();
	const int dstCount = buffer.getValidByteCount();
	if (srcCount != dstCount)
	{
		DebugLogError("Mismatched uniform buffer sizes for ID " + std::to_string(id) + ": " +
			std::to_string(srcCount) + " != " + std::to_string(dstCount));
		return;
	}

	const std::byte *srcBegin = data.begin();
	const std::byte *srcEnd = srcBegin + srcCount;
	std::copy(srcBegin, srcEnd, buffer.begin());
}

void SoftwareRenderer::populateUniformAtIndex(UniformBufferID id, int uniformIndex, BufferView<const std::byte> uniformData)
{
	UniformBuffer &buffer = this->uniformBuffers.get(id);
	const int srcByteCount = uniformData.getCount();
	const int dstByteCount = static_cast<int>(buffer.sizeOfElement);
	if (srcByteCount != dstByteCount)
	{
		DebugLogError("Mismatched uniform size for uniform buffer ID " + std::to_string(id) + " index " +
			std::to_string(uniformIndex) + ": " + std::to_string(srcByteCount) + " != " + std::to_string(dstByteCount));
		return;
	}

	const std::byte *srcBegin = uniformData.begin();
	const std::byte *srcEnd = srcBegin + srcByteCount;
	std::byte *dstBegin = buffer.begin() + (dstByteCount * uniformIndex);
	std::copy(srcBegin, srcEnd, dstBegin);
}

void SoftwareRenderer::freeUniformBuffer(UniformBufferID id)
{
	this->uniformBuffers.free(id);
}

bool SoftwareRenderer::tryCreateLight(RenderLightID *outID)
{
	if (!this->lights.tryAlloc(outID))
	{
		DebugLogError("Couldn't allocate render light ID.");
		return false;
	}

	return true;
}

void SoftwareRenderer::setLightPosition(RenderLightID id, const Double3 &worldPoint)
{
	Light &light = this->lights.get(id);
	light.worldPointX = worldPoint.x;
	light.worldPointY = worldPoint.y;
	light.worldPointZ = worldPoint.z;
}

void SoftwareRenderer::setLightRadius(RenderLightID id, double startRadius, double endRadius)
{
	DebugAssert(startRadius >= 0.0);
	DebugAssert(endRadius >= startRadius);
	Light &light = this->lights.get(id);
	light.startRadius = startRadius;
	light.startRadiusSqr = startRadius * startRadius;
	light.endRadius = endRadius;
	light.endRadiusSqr = endRadius * endRadius;
	light.startEndRadiusDiff = endRadius - startRadius;
	light.startEndRadiusDiffRecip = 1.0 / light.startEndRadiusDiff;
}

void SoftwareRenderer::freeLight(RenderLightID id)
{
	this->lights.free(id);
}

Renderer3DProfilerData SoftwareRenderer::getProfilerData() const
{
	const int renderWidth = this->paletteIndexBuffer.getWidth();
	const int renderHeight = this->paletteIndexBuffer.getHeight();
	const int threadCount = g_workers.getCount();
	const int drawCallCount = g_totalDrawCallCount;
	const int presentedTriangleCount = g_totalPresentedTriangleCount;

	const int textureCount = this->objectTextures.getUsedCount();
	int textureByteCount = 0;
	for (int i = 0; i < this->objectTextures.getTotalCount(); i++)
	{
		const ObjectTextureID id = static_cast<ObjectTextureID>(i);
		const ObjectTexture *texturePtr = this->objectTextures.tryGet(id);
		if (texturePtr != nullptr)
		{
			textureByteCount += texturePtr->texels.getCount();
		}
	}

	const int totalLightCount = this->lights.getUsedCount();
	const int totalCoverageTests = g_totalCoverageTests;
	const int totalDepthTests = g_totalDepthTests;
	const int totalColorWrites = g_totalColorWrites;

	return Renderer3DProfilerData(renderWidth, renderHeight, threadCount, drawCallCount, presentedTriangleCount,
		textureCount, textureByteCount, totalLightCount, totalCoverageTests, totalDepthTests, totalColorWrites);
}

void SoftwareRenderer::submitFrame(const RenderCamera &camera, const RenderFrameSettings &settings,
	const RenderCommandBuffer &commandBuffer, uint32_t *outputBuffer)
{
	const int totalDrawCallCount = commandBuffer.getTotalDrawCallCount();
	const int frameBufferWidth = this->paletteIndexBuffer.getWidth();
	const int frameBufferHeight = this->paletteIndexBuffer.getHeight();

	if (this->ditheringMode != settings.ditheringMode)
	{
		this->ditheringMode = settings.ditheringMode;
		CreateDitherBuffer(this->ditherBuffer, frameBufferWidth, frameBufferHeight, settings.ditheringMode);
	}

	const ObjectTexture &paletteTexture = this->objectTextures.get(settings.paletteTextureID);
	const ObjectTexture &lightTableTexture = this->objectTextures.get(settings.lightTableTextureID);
	const ObjectTexture &skyBgTexture = this->objectTextures.get(settings.skyBgTextureID);

	PopulateCameraGlobals(camera);
	PopulateDrawCallGlobals(totalDrawCallCount);
	PopulateRasterizerGlobals(frameBufferWidth, frameBufferHeight, this->paletteIndexBuffer.begin(), this->depthBuffer.begin(),
		this->ditherBuffer.begin(), this->ditherBuffer.getDepth(), this->ditheringMode, outputBuffer, &this->objectTextures);
	PopulatePixelShaderGlobals(settings.ambientPercent, settings.screenSpaceAnimPercent, paletteTexture, lightTableTexture, skyBgTexture);

	const int totalWorkerCount = RendererUtils::getRenderThreadsFromMode(settings.renderThreadsMode);
	InitializeWorkers(totalWorkerCount, frameBufferWidth, frameBufferHeight);

	ClearTriangleTotalCounts();
	ClearFrameBuffers();

	std::unique_lock<std::mutex> lock(g_mutex);

	for (int commandIndex = 0; commandIndex < commandBuffer.entryCount; commandIndex++)
	{
		const BufferView<const RenderDrawCall> drawCalls = commandBuffer.entries[commandIndex];
		int startDrawCallIndex = 0;
		int remainingDrawCallCount = drawCalls.getCount();
		constexpr int maxDrawCallsPerLoop = 8192;
		static_assert(maxDrawCallsPerLoop <= MAX_WORKER_DRAW_CALLS_PER_LOOP);

		while (remainingDrawCallCount > 0)
		{
			// Wait for all workers to be ready to process this set of draw calls.
			g_directorCondVar.wait(lock, []()
			{
				return std::all_of(g_workers.begin(), g_workers.end(), [](const Worker &worker) { return worker.isReadyToStartFrame; });
			});

			for (Worker &worker : g_workers)
			{
				DebugAssert(!worker.shouldExit);
				DebugAssert(!worker.shouldWorkOnDrawCalls);
				DebugAssert(!worker.isFinishedWithDrawCalls);
				DebugAssert(!worker.shouldWorkOnRasterizing);
				DebugAssert(!worker.isFinishedRasterizing);
				worker.isReadyToStartFrame = false;
				worker.rasterizerInputCache.clearTriangles();
				worker.rasterizerInputCache.emptyBins();
			}

			// Determine which workers get which draw calls this loop.
			const int drawCallsToConsume = std::min(maxDrawCallsPerLoop, remainingDrawCallCount);
			PopulateWorkerDrawCallWorkloads(totalWorkerCount, startDrawCallIndex, drawCallsToConsume);

			// Populate worker draw call caches so they have data to work with.
			for (Worker &worker : g_workers)
			{
				for (int workerDrawCallIndex = 0; workerDrawCallIndex < worker.drawCallCount; workerDrawCallIndex++)
				{
					const int globalDrawCallIndex = worker.drawCallStartIndex + workerDrawCallIndex;
					const RenderDrawCall &drawCall = drawCalls[globalDrawCallIndex];

					DebugAssertIndex(worker.drawCallCaches, workerDrawCallIndex);
					DrawCallCache &workerDrawCallCache = worker.drawCallCaches[workerDrawCallIndex];
					TransformCache &workerTransformCache = worker.transformCaches[workerDrawCallIndex];
					auto &transformCachePreScaleTranslationX = workerTransformCache.preScaleTranslationX;
					auto &transformCachePreScaleTranslationY = workerTransformCache.preScaleTranslationY;
					auto &transformCachePreScaleTranslationZ = workerTransformCache.preScaleTranslationZ;
					auto &drawCallCacheVertexBuffer = workerDrawCallCache.vertexBuffer;
					auto &drawCallCacheTexCoordBuffer = workerDrawCallCache.texCoordBuffer;
					auto &drawCallCacheIndexBuffer = workerDrawCallCache.indexBuffer;
					auto &drawCallCacheTextureID0 = workerDrawCallCache.textureID0;
					auto &drawCallCacheTextureID1 = workerDrawCallCache.textureID1;
					auto &drawCallCacheLightingType = workerDrawCallCache.lightingType;
					auto &drawCallCacheMeshLightPercent = workerDrawCallCache.meshLightPercent;
					auto &drawCallCacheLightPtrArray = workerDrawCallCache.lightPtrArray;
					auto &drawCallCacheLightCount = workerDrawCallCache.lightCount;
					auto &drawCallCacheVertexShaderType = workerDrawCallCache.vertexShaderType;
					auto &drawCallCachePixelShaderType = workerDrawCallCache.pixelShaderType;
					auto &drawCallCachePixelShaderParam0 = workerDrawCallCache.pixelShaderParam0;
					auto &drawCallCacheEnableDepthRead = workerDrawCallCache.enableDepthRead;
					auto &drawCallCacheEnableDepthWrite = workerDrawCallCache.enableDepthWrite;

					const UniformBuffer &transformBuffer = this->uniformBuffers.get(drawCall.transformBufferID);
					const RenderTransform &transform = transformBuffer.get<RenderTransform>(drawCall.transformIndex);
					PopulateMeshTransform(workerTransformCache, transform);

					transformCachePreScaleTranslationX = 0.0;
					transformCachePreScaleTranslationY = 0.0;
					transformCachePreScaleTranslationZ = 0.0;
					if (drawCall.preScaleTranslationBufferID >= 0)
					{
						const UniformBuffer &preScaleTranslationBuffer = this->uniformBuffers.get(drawCall.preScaleTranslationBufferID);
						const Double3 &preScaleTranslation = preScaleTranslationBuffer.get<Double3>(0);
						transformCachePreScaleTranslationX = preScaleTranslation.x;
						transformCachePreScaleTranslationY = preScaleTranslation.y;
						transformCachePreScaleTranslationZ = preScaleTranslation.z;
					}

					drawCallCacheVertexBuffer = &this->vertexBuffers.get(drawCall.vertexBufferID);
					drawCallCacheTexCoordBuffer = &this->attributeBuffers.get(drawCall.texCoordBufferID);
					drawCallCacheIndexBuffer = &this->indexBuffers.get(drawCall.indexBufferID);
					drawCallCacheTextureID0 = drawCall.textureIDs[0];
					drawCallCacheTextureID1 = drawCall.textureIDs[1];
					drawCallCacheLightingType = drawCall.lightingType;
					drawCallCacheMeshLightPercent = drawCall.lightPercent;

					auto &drawCallCacheLightPtrs = drawCallCacheLightPtrArray;
					for (int lightIndex = 0; lightIndex < drawCall.lightIdCount; lightIndex++)
					{
						const RenderLightID lightID = drawCall.lightIDs[lightIndex];
						drawCallCacheLightPtrs[lightIndex] = &this->lights.get(lightID);
					}

					drawCallCacheLightCount = drawCall.lightIdCount;
					drawCallCacheVertexShaderType = drawCall.vertexShaderType;
					drawCallCachePixelShaderType = drawCall.pixelShaderType;
					drawCallCachePixelShaderParam0 = drawCall.pixelShaderParam0;
					drawCallCacheEnableDepthRead = drawCall.enableDepthRead;
					drawCallCacheEnableDepthWrite = drawCall.enableDepthWrite;
				}
			}

			for (Worker &worker : g_workers)
			{
				DebugAssert(!worker.shouldWorkOnDrawCalls);
				worker.shouldWorkOnDrawCalls = true;
			}

			g_workerCondVar.notify_all();
			g_directorCondVar.wait(lock, []()
			{
				return std::all_of(g_workers.begin(), g_workers.end(), [](const Worker &worker) { return worker.isFinishedWithDrawCalls; });
			});

			for (Worker &worker : g_workers)
			{
				DebugAssert(worker.shouldWorkOnDrawCalls);
				DebugAssert(!worker.shouldWorkOnRasterizing);
				DebugAssert(!worker.isFinishedRasterizing);
				worker.shouldWorkOnDrawCalls = false;
				worker.shouldWorkOnRasterizing = true;
				g_totalPresentedTriangleCount += worker.rasterizerInputCache.triangleCount;
			}

			g_workerCondVar.notify_all();
			g_directorCondVar.wait(lock, []()
			{
				return std::all_of(g_workers.begin(), g_workers.end(), [](const Worker &worker) { return worker.isFinishedRasterizing; });
			});

			// Reset workers for next frame.
			for (Worker &worker : g_workers)
			{
				worker.isFinishedWithDrawCalls = false;
				worker.shouldWorkOnRasterizing = false;
				worker.isFinishedRasterizing = false;
			}

			startDrawCallIndex += drawCallsToConsume;
			remainingDrawCallCount -= drawCallsToConsume;
		}
	}
}

void SoftwareRenderer::present()
{
	// Do nothing for now, might change later.
}

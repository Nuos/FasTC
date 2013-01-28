/* FasTC
 * Copyright (c) 2012 University of North Carolina at Chapel Hill. All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software and its documentation for educational, 
 * research, and non-profit purposes, without fee, and without a written agreement is hereby granted, 
 * provided that the above copyright notice, this paragraph, and the following four paragraphs appear 
 * in all copies.
 *
 * Permission to incorporate this software into commercial products may be obtained by contacting the 
 * authors or the Office of Technology Development at the University of North Carolina at Chapel Hill <otd@unc.edu>.
 *
 * This software program and documentation are copyrighted by the University of North Carolina at Chapel Hill. 
 * The software program and documentation are supplied "as is," without any accompanying services from the 
 * University of North Carolina at Chapel Hill or the authors. The University of North Carolina at Chapel Hill 
 * and the authors do not warrant that the operation of the program will be uninterrupted or error-free. The 
 * end-user understands that the program was developed for research purposes and is advised not to rely 
 * exclusively on the program for any reason.
 *
 * IN NO EVENT SHALL THE UNIVERSITY OF NORTH CAROLINA AT CHAPEL HILL OR THE AUTHORS BE LIABLE TO ANY PARTY FOR 
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING LOST PROFITS, ARISING OUT OF THE 
 * USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF NORTH CAROLINA AT CHAPEL HILL OR THE 
 * AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * THE UNIVERSITY OF NORTH CAROLINA AT CHAPEL HILL AND THE AUTHORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING, 
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE AND ANY 
 * STATUTORY WARRANTY OF NON-INFRINGEMENT. THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS" BASIS, AND THE UNIVERSITY 
 * OF NORTH CAROLINA AT CHAPEL HILL AND THE AUTHORS HAVE NO OBLIGATIONS TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, 
 * ENHANCEMENTS, OR MODIFICATIONS.
 *
 * Please send all BUG REPORTS to <pavel@cs.unc.edu>.
 *
 * The authors may be contacted via:
 *
 * Pavel Krajcevski
 * Dept of Computer Science
 * 201 S Columbia St
 * Frederick P. Brooks, Jr. Computer Science Bldg
 * Chapel Hill, NC 27599-3175
 * USA
 * 
 * <http://gamma.cs.unc.edu/FasTC/>
 */

// The original lisence from the code available at the following location:
// http://software.intel.com/en-us/vcsource/samples/fast-texture-compression
//
// This code has been modified significantly from the original.

//--------------------------------------------------------------------------------------
// Copyright 2011 Intel Corporation
// All Rights Reserved
//
// Permission is granted to use, copy, distribute and prepare derivative works of this
// software for any purpose and without fee, provided, that the above copyright notice
// and this statement appear in all copies.  Intel makes no representations about the
// suitability of this software for any purpose.  THIS SOFTWARE IS PROVIDED "AS IS."
// INTEL SPECIFICALLY DISCLAIMS ALL WARRANTIES, EXPRESS OR IMPLIED, AND ALL LIABILITY,
// INCLUDING CONSEQUENTIAL AND OTHER INDIRECT DAMAGES, FOR THE USE OF THIS SOFTWARE,
// INCLUDING LIABILITY FOR INFRINGEMENT OF ANY PROPRIETARY RIGHTS, AND INCLUDING THE
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  Intel does not
// assume any responsibility for any errors which may appear in this software nor any
// responsibility to update it.
//
//--------------------------------------------------------------------------------------

#ifndef __BC7_COMPRESSIONMODE_SIMD_H__
#define __BC7_COMPRESSIONMODE_SIMD_H__

#include "RGBAEndpoints.h"

// Forward Declarations
class BitStream;
const int kMaxEndpoints = 3;

static const int kPBits[4][2] = {
	{ 0, 0 },
	{ 0, 1 },
	{ 1, 0 },
	{ 1, 1 }
};

// Abstract class that outlines all of the different settings for BC7 compression modes 
// Note that at the moment, we only support modes 0-3, so we don't deal with alpha channels.
class BC7CompressionMode {
public:

	static const uint32 kMaxNumSubsets = 3;
	static const uint32 kNumModes = 8;

	explicit BC7CompressionMode(int mode, bool opaque = true) : m_IsOpaque(opaque), m_Attributes(&(kModeAttributes[mode])), m_RotateMode(0), m_IndexMode(0) { }
	~BC7CompressionMode() { }

	double Compress(BitStream &stream, const int shapeIdx, const RGBACluster *clusters);

	// This switch controls the quality of the simulated annealing optimizer. We will not make
	// more than this many steps regardless of how bad the error is. Higher values will produce
	// better quality results but will run slower. Default is 20.
	static int MaxAnnealingIterations; // This is a setting
	static const int kMaxAnnealingIterations = 256; // This is a limit

	enum EPBitType {
		ePBitType_Shared,
		ePBitType_NotShared,
		ePBitType_None
	};

	static struct Attributes {
		int modeNumber;
		int numPartitionBits;
		int numSubsets;
		int numBitsPerIndex;
		int numBitsPerAlpha;
		int colorChannelPrecision;
		int alphaChannelPrecision;
		bool hasRotation;
		bool hasIdxMode;
		EPBitType pbitType;
	} kModeAttributes[kNumModes];

	static const Attributes *GetAttributesForMode(int mode) {
		if(mode < 0 || mode >= 8) return NULL;
		return &kModeAttributes[mode];
	}

private:
	
        const double m_IsOpaque;
	const Attributes *const m_Attributes;

	int m_RotateMode;
	int m_IndexMode;

	void SetIndexMode(int mode) { m_IndexMode = mode; }
	void SetRotationMode(int mode) { m_RotateMode = mode; }

	int GetRotationMode() const { return m_Attributes->hasRotation? m_RotateMode : 0; }

	int GetModeNumber() const { return m_Attributes->modeNumber; }
	int GetNumberOfPartitionBits() const { return m_Attributes->numPartitionBits; }
	int GetNumberOfSubsets() const { return m_Attributes->numSubsets; }

	int GetNumberOfBitsPerIndex(int indexMode = -1) const { 
		if(indexMode < 0) indexMode = m_IndexMode;
		if(indexMode == 0)
			return m_Attributes->numBitsPerIndex; 
		else
			return m_Attributes->numBitsPerAlpha; 
	}

	int GetNumberOfBitsPerAlpha(int indexMode = -1) const { 
		if(indexMode < 0) indexMode = m_IndexMode;
		if(indexMode == 0)
			return m_Attributes->numBitsPerAlpha; 
		else
			return m_Attributes->numBitsPerIndex; 
	}

	// If we handle alpha separately, then we will consider the alpha channel
	// to be not used whenever we do any calculations...
	int GetAlphaChannelPrecision() const { 
          return m_Attributes->alphaChannelPrecision;  
	}

	RGBAVector GetErrorMetric() const {
		const float *w = BC7C::GetErrorMetric();
		switch(GetRotationMode()) {
			default:
			case 0: return RGBAVector(w[0], w[1], w[2], w[3]);
			case 1: return RGBAVector(w[3], w[1], w[2], w[0]);
			case 2: return RGBAVector(w[0], w[3], w[2], w[1]);
			case 3: return RGBAVector(w[0], w[1], w[3], w[2]);
		}
	}

	EPBitType GetPBitType() const { return m_Attributes->pbitType; }

	unsigned int GetQuantizationMask() const {	
		const int maskSeed = 0x80000000;
                const uint32 alphaPrec = GetAlphaChannelPrecision();
                if(alphaPrec > 0) {
                  return (
                    (maskSeed >> (24 + m_Attributes->colorChannelPrecision - 1) & 0xFF) |
                    (maskSeed >> (16 + m_Attributes->colorChannelPrecision - 1) & 0xFF00) |
                    (maskSeed >> (8 + m_Attributes->colorChannelPrecision - 1) & 0xFF0000) |
                    (maskSeed >> (GetAlphaChannelPrecision() - 1) & 0xFF000000)
                  );
                }
                else {
                  return (
                    (maskSeed >> (24 + m_Attributes->colorChannelPrecision - 1) & 0xFF) |
                    (maskSeed >> (16 + m_Attributes->colorChannelPrecision - 1) & 0xFF00) |
                    (maskSeed >> (8 + m_Attributes->colorChannelPrecision - 1) & 0xFF0000) &
                    (0x00FFFFFF)
                  );
                }
	}

	int GetNumPbitCombos() const {
		switch(GetPBitType()) {
			case ePBitType_Shared: return 2;
			case ePBitType_NotShared: return 4;
			default:
			case ePBitType_None: return 1;
		}
	}

	const int *GetPBitCombo(int idx) const {
		switch(GetPBitType()) {
			case ePBitType_Shared: return (idx)? kPBits[3] : kPBits[0];
			case ePBitType_NotShared: return kPBits[idx % 4];
			default:
			case ePBitType_None: return kPBits[0];
		}
	}
	
	double OptimizeEndpointsForCluster(const RGBACluster &cluster, RGBAVector &p1, RGBAVector &p2, int *bestIndices, int &bestPbitCombo) const;

	struct VisitedState {
		RGBAVector p1;
		RGBAVector p2;
		int pBitCombo;
	};

	void PickBestNeighboringEndpoints(
		const RGBACluster &cluster, 
		const RGBAVector &p1, const RGBAVector &p2, 
		const int curPbitCombo, 
		RGBAVector &np1, RGBAVector &np2, 
		int &nPbitCombo, 
		const VisitedState *visitedStates, 
		int nVisited, 
		float stepSz = 1.0f
	) const;

	bool AcceptNewEndpointError(double newError, double oldError, float temp) const;

	double CompressSingleColor(const RGBAVector &p, RGBAVector &p1, RGBAVector &p2, int &bestPbitCombo) const;
	double CompressCluster(const RGBACluster &cluster, RGBAVector &p1, RGBAVector &p2, int *bestIndices, int &bestPbitCombo) const;
	double CompressCluster(const RGBACluster &cluster, RGBAVector &p1, RGBAVector &p2, int *bestIndices, int *alphaIndices) const;

	void ClampEndpointsToGrid(RGBAVector &p1, RGBAVector &p2, int &bestPBitCombo) const;
};

extern const uint32 kBC7InterpolationValues[4][16][2];

#endif // __BC7_COMPRESSIONMODE_SIMD_H__

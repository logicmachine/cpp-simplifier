// clang-format off
/***************************************************************************/
/*  Copyright (c) 2022 Dhruv Makwana                                       */
/*  All rights reserved.                                                   */
/*                                                                         */
/*  This software was developed by the University of Cambridge Computer    */
/*  Laboratory as part of the Rigorous Engineering of Mainstream Systems   */
/*  (REMS) project. This project has been partly funded by an EPSRC        */
/*  Doctoral Training studentship. This project has been partly funded by  */
/*  Google. This project has received funding from the European Research   */
/*  Council (ERC) under the European Union's Horizon 2020 research and     */
/*  innovation programme (grant agreement No 789108, Advanced Grant        */
/*  ELVER).                                                                */
/*                                                                         */
/*  BSD 2-Clause License                                                   */
/*                                                                         */
/*  Redistribution and use in source and binary forms, with or without     */
/*  modification, are permitted provided that the following conditions     */
/*  are met:                                                               */
/*  1. Redistributions of source code must retain the above copyright      */
/*     notice, this list of conditions and the following disclaimer.       */
/*  2. Redistributions in binary form must reproduce the above copyright   */
/*     notice, this list of conditions and the following disclaimer in     */
/*     the documentation and/or other materials provided with the          */
/*     distribution.                                                       */
/*                                                                         */
/*  THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS''     */
/*  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED      */
/*  TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A        */
/*  PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR    */
/*  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,           */
/*  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT       */
/*  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF       */
/*  USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND    */
/*  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,     */
/*  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT     */
/*  OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF     */
/*  SUCH DAMAGE.                                                           */
/***************************************************************************/

// clang-format on
#include <clang/Basic/SourceLocation.h>
#include <unordered_set>

#pragma once

struct SourceRangeHash {
    size_t operator()(const clang::SourceRange &range) const {
        size_t result = std::hash<unsigned>()(range.getBegin().getRawEncoding());
        result <<= sizeof(unsigned) * 8;
        result |= std::hash<unsigned>()(range.getEnd().getRawEncoding());
        return result;
    }
};

using SourceRangeSet = std::unordered_set<clang::SourceRange, SourceRangeHash>;

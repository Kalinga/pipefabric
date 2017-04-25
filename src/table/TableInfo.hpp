/*
 * Copyright (c) 2014-17 The PipeFabric team,
 *                       All Rights Reserved.
 *
 * This file is part of the PipeFabric package.
 *
 * PipeFabric is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License (GPL) as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file LICENSE.
 * If not you can find the GPL at http://www.gnu.org/copyleft/gpl.html
 */

#ifndef TableInfo_hpp_
#define TableInfo_hpp_

#if defined(USE_NVML_TABLE)

#include "nvm/PTableInfo.hpp"

namespace pfabric {
using TableInfo = pfabric::nvm::PTableInfo;
using ColumnInfo = pfabric::nvm::ColumnInfo;
}

#else

#include <table/VTableInfo.hpp>

namespace pfabric {
using TableInfo = pfabric::VTableInfo;
using ColumnInfo = pfabric::ColumnInfo;
}

#endif

#endif


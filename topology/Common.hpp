/**
 * @file Common.hpp
 * @author bwu
 * @brief Common define of topology
 * @version 0.1
 * @date 2022-02-22
 */
#ifndef GENERIC_TOPOLOGY_COMMON_HPP
#define GENERIC_TOPOLOGY_COMMON_HPP
#include <utility>
namespace generic {
///@brief graph related concepts
namespace topology{

using index_t = std::size_t;
inline static constexpr index_t noIndex = std::numeric_limits<index_t>::max();

}//namespace topology
}//namespace generic
#endif//GENERIC_TOPOLOGY_COMMON_HPP
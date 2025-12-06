/**
 * test_factory.h - Factory for creating layout strategies in tests
 */

#pragma once

#include "text/layout/GreedyLayoutStrategy.h"
#include "text/layout/KnuthPlassLayoutStrategy.h"
#include "text/layout/LayoutStrategy.h"

namespace TestFactory {

enum class LayoutType { GREEDY, KNUTH_PLASS };

// Default layout type for tests
inline LayoutType g_defaultLayoutType = LayoutType::KNUTH_PLASS;

inline const char* layoutTypeName(LayoutType type) {
  switch (type) {
    case LayoutType::GREEDY:
      return "GreedyLayoutStrategy";
    case LayoutType::KNUTH_PLASS:
      return "KnuthPlassLayoutStrategy";
    default:
      return "Unknown";
  }
}

inline LayoutStrategy* createLayout(LayoutType type) {
  switch (type) {
    case LayoutType::GREEDY:
      return new GreedyLayoutStrategy();
    case LayoutType::KNUTH_PLASS:
      return new KnuthPlassLayoutStrategy();
    default:
      return nullptr;
  }
}

}  // namespace TestFactory

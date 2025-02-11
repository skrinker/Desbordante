#include "LatticeObservations.h"

NodeCategory LatticeObservations::updateDependencyCategory(Vertical const& vertical) {
    //бежим по множеству подмножеств и смотрим. если получили все независимости, то это мин зависимость

    NodeCategory newCategory;
    if (vertical.getArity() <= 1) {
        newCategory = NodeCategory::minimalDependency;
        (*this)[vertical] = newCategory;
        return newCategory;
    }

    auto columnIndices = vertical.getColumnIndicesRef(); //копируем индексы
    bool hasUncheckedSubset = false;

    for (size_t index = columnIndices.find_first();
         index < columnIndices.size();
         index = columnIndices.find_next(index)
    ) {
        columnIndices[index] = false; //убираем одну из колонок
        auto const subsetVerticalIter = this->find(Vertical(vertical.getSchema(), columnIndices));

        //если какое-то подмножество не посещено либо оно не является антизависимостью, то не подходит

        if (subsetVerticalIter == this->end()) {
            //если нашли нерассмотренное подмножество
            hasUncheckedSubset = true;
        } else {
            NodeCategory const &subsetVerticalCategory = subsetVerticalIter->second;
            if (subsetVerticalCategory == NodeCategory::minimalDependency ||
                subsetVerticalCategory == NodeCategory::dependency ||
                subsetVerticalCategory == NodeCategory::candidateMinimalDependency
            ) {
                newCategory = NodeCategory::dependency;
                (*this)[vertical] = newCategory;
                return newCategory;
            }
        }
        //если все нормально
        columnIndices[index] = true; //возвращаем как было
    }
    newCategory = hasUncheckedSubset ? NodeCategory::candidateMinimalDependency : NodeCategory::minimalDependency;
    (*this)[vertical] = newCategory;
    return newCategory;
}

NodeCategory LatticeObservations::updateNonDependencyCategory(Vertical const& vertical, int rhsIndex) {
    auto columnIndices = vertical.getColumnIndicesRef();
    columnIndices[rhsIndex] = true;
    columnIndices.flip();

    NodeCategory newCategory;
    bool hasUncheckedSuperset = false;

    for (size_t index = columnIndices.find_first();
         index < columnIndices.size();
         index = columnIndices.find_next(index)
    ) {
        auto const supersetVerticalIter = this->find(vertical.Union(*vertical.getSchema()->getColumn(index)));

        if (supersetVerticalIter == this->end()) {
            //если нашли нерассмотренное надмножество
            hasUncheckedSuperset = true;
        } else {
            NodeCategory const &supersetVerticalCategory = supersetVerticalIter->second;
            if (supersetVerticalCategory == NodeCategory::maximalNonDependency ||
                supersetVerticalCategory == NodeCategory::nonDependency ||
                supersetVerticalCategory == NodeCategory::candidateMaximalNonDependency
            ) {
                newCategory = NodeCategory::nonDependency;
                (*this)[vertical] = newCategory;
                return newCategory;
            }
        }
    }
    newCategory = hasUncheckedSuperset ? NodeCategory::candidateMaximalNonDependency : NodeCategory::maximalNonDependency;
    (*this)[vertical] = newCategory;
    return newCategory;
}

bool LatticeObservations::isCandidate(Vertical const& node) const {
    auto nodeIter = this->find(node);
    if (nodeIter == this->end()) {
        return false;
    } else {
        return nodeIter->second == NodeCategory::candidateMaximalNonDependency ||
               nodeIter->second == NodeCategory::candidateMinimalDependency;
    }
}

std::unordered_set<Vertical>
LatticeObservations::getUncheckedSubsets(Vertical const& node, ColumnOrder const& columnOrder) const {
    auto indices = node.getColumnIndices();
    std::unordered_set<Vertical> uncheckedSubsets;

    for (int columnIndex : columnOrder.getOrderHighDistinctCount(node)) {
        indices[columnIndex] = false;
        Vertical subsetNode = Vertical(node.getSchema(), indices);
        if (this->find(subsetNode) == this->end()) {
            uncheckedSubsets.insert(std::move(subsetNode));
        }
        indices[columnIndex] = true;
    }

    return uncheckedSubsets;
}

std::unordered_set<Vertical>
LatticeObservations::getUncheckedSupersets(Vertical const& node, size_t rhsIndex, ColumnOrder const& columnOrder) const {
    auto flippedIndices = node.getColumnIndices().flip();
    std::unordered_set<Vertical> uncheckedSupersets;

    flippedIndices[rhsIndex] = false;

    for (int columnIndex : columnOrder.getOrderHighDistinctCount(Vertical(node.getSchema(), flippedIndices))) {
        auto indices = node.getColumnIndices();

        indices[columnIndex] = true;
        Vertical subsetNode = Vertical(node.getSchema(), indices);
        if (this->find(subsetNode) == this->end()) {
            uncheckedSupersets.insert(std::move(subsetNode));
        }
    }

    return uncheckedSupersets;
}

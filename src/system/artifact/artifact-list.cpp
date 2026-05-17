#include "system/artifact/artifact-list.h"
#include "artifact/fixed-art-types.h"
#include "system/artifact/artifact-definition.h"

ArtifactList ArtifactList::instance{};

ArtifactDefinition ArtifactList::dummy{};

ArtifactList &ArtifactList::get_instance()
{
    return instance;
}

const ArtifactDefinition &ArtifactList::get_artifact(const FixedArtifactId fa_id) const
{
    if (fa_id == FixedArtifactId::NONE) {
        return dummy;
    }

    return this->artifacts.at(fa_id);
}

bool ArtifactList::order(const FixedArtifactId id1, const FixedArtifactId id2) const
{
    const auto &artifact1 = this->get_artifact(id1);
    const auto &artifact2 = this->get_artifact(id2);
    if (artifact1.bi_key < artifact2.bi_key) {
        return true;
    }

    if (artifact1.bi_key > artifact2.bi_key) {
        return false;
    }

    if (artifact1.level < artifact2.level) {
        return true;
    }

    if (artifact1.level > artifact2.level) {
        return false;
    }

    return id1 < id2;
}

void ArtifactList::emplace(const FixedArtifactId fa_id, ArtifactDefinition &&artifact)
{
    this->artifacts.emplace(fa_id, std::move(artifact));
}

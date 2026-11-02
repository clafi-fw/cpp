export module ClaFi.Core.DomEngine :Defaults;

import :Core;
import :MultiTypeSequence;
import :Section;
import :Sequence;
import ClaFi.StdLib;

namespace ClaFi::Dom
{
    // Whether two nodes would be written the same way. A scalar is compared as the text a format
    // puts in the file, so two that agree there agree for every format at once.
    export [[nodiscard]] bool sameValue(const DomNodeBase&, const DomNodeBase&);

    // A copy of section carrying only what differs from defaults: a child that would be written
    // the same way is left out, and so is a section every one of whose children was left out.
    //
    // The result is for writing. A section's children are its schema - that is why nothing can be
    // removed from a live tree, since a tree missing a child has nowhere to put what a file says
    // about it. Nothing is removed here either: the copy is built from the children that differ.
    export [[nodiscard]] std::unique_ptr<Section> withoutDefaults(const Section&,
        const Section& defaults);


    //-------------------------------------------------------------------------


    // Whether a node holds other nodes by name. A composite value is a section that also knows
    // the type it was read from, and the two are the same thing to a writer.
    [[nodiscard]] bool isSectionLike(DomNodeType type)
    {
        return type == DomNodeType::Section or type == DomNodeType::CompositeValue;
    }

    bool sameValue(const DomNodeBase& node, const DomNodeBase& other)
    {
        if (isSectionLike(node.type()) and isSectionLike(other.type()))
        {
            const Section& section = static_cast<const Section&>(node);
            const Section& otherSection = static_cast<const Section&>(other);
            const std::vector<std::wstring_view> keys = section.getKeys();
            if (keys.size() != otherSection.getKeys().size())
                return false;
            for (const std::wstring_view key : keys)
            {
                const DomNodeBase* otherChild = otherSection.child(key);
                if (!otherChild)
                    return false;
                if (!sameValue(*section.child(key), *otherChild))
                    return false;
            }
            return true;
        }

        if (node.type() != other.type())
            return false;

        if (node.type() == DomNodeType::ScalarValue)
            return static_cast<const ScalarValueBase&>(node).getAsRaw()
                == static_cast<const ScalarValueBase&>(other).getAsRaw();

        if (node.type() == DomNodeType::Sequence)
        {
            // A sequence is answered whole. Its items are read back by position, so leaving one
            // out would move every item after it.
            const SequenceBase& sequence = static_cast<const SequenceBase&>(node);
            const SequenceBase& otherSequence = static_cast<const SequenceBase&>(other);
            if (sequence.size() != otherSequence.size())
                return false;
            for (std::size_t i = 0ull; i != sequence.size(); ++i)
                if (!sameValue(*sequence.child(i), *otherSequence.child(i)))
                    return false;
            return true;
        }

        if (node.type() == DomNodeType::MultiTypeSequence)
        {
            // Answered whole, as a sequence is: an item left out would move every item after it.
            const MultiTypeSequence::Items& items =
                static_cast<const MultiTypeSequence&>(node).items();
            const MultiTypeSequence::Items& otherItems =
                static_cast<const MultiTypeSequence&>(other).items();
            if (items.size() != otherItems.size())
                return false;
            for (std::size_t i = 0ull; i != items.size(); ++i)
            {
                if (items[i].name != otherItems[i].name)
                    return false;
                if (!sameValue(*items[i].node, *otherItems[i].node))
                    return false;
            }
            return true;
        }

        return false;
    }

    // Every item stays, even one stating nothing, since a list is read back by position.
    [[nodiscard]] std::unique_ptr<DomNodeBase> itemsWithoutDefaults(const SequenceBase& sequence)
    {
        std::unique_ptr<DomNodeBase> copy = sequence.clone(nullptr);
        const std::unique_ptr<DomNodeBase> defaultItem = sequence.makeDefaultItem();
        if (!isSectionLike(defaultItem->type()))
            return copy;

        const Section& defaults = static_cast<const Section&>(*defaultItem);
        for (std::unique_ptr<DomNodeBase>& item : static_cast<SequenceBase&>(*copy).children())
        {
            if (!isSectionLike(item->type()))
                continue;
            std::unique_ptr<Section> stated = withoutDefaults(static_cast<const Section&>(*item),
                defaults);
            stated->setParent(copy.get());
            item = std::move(stated);
        }
        return copy;
    }

    // Every item stays, each carrying only what differs from its name's blueprint.
    [[nodiscard]] std::unique_ptr<DomNodeBase> itemsWithoutDefaults(const MultiTypeSequence& list)
    {
        std::unique_ptr<DomNodeBase> copy = list.clone(nullptr);
        MultiTypeSequence& copiedList = static_cast<MultiTypeSequence&>(*copy);
        copiedList.clear();
        for (const MultiTypeSequence::Item& item : list.items())
        {
            const std::unique_ptr<DomNodeBase> defaultItem = list.makeDefaultItem(item.name);
            const bool bothSections = defaultItem && isSectionLike(item.node->type())
                && isSectionLike(defaultItem->type());
            std::unique_ptr<DomNodeBase> stated = bothSections
                ? withoutDefaults(static_cast<const Section&>(*item.node),
                    static_cast<const Section&>(*defaultItem))
                : item.node->clone(nullptr);
            copiedList.addItem(item.name, std::move(stated));
        }
        return copy;
    }

    std::unique_ptr<Section> withoutDefaults(const Section& section, const Section& defaults)
    {
        std::unique_ptr<Section> result = std::make_unique<Section>(nullptr);
        for (const std::wstring_view key : section.getKeys())
        {
            const DomNodeBase& child = *section.child(key);
            const DomNodeBase* defaultChild = defaults.child(key);
            if (defaultChild and sameValue(child, *defaultChild))
                continue;

            std::unique_ptr<DomNodeBase> copy;
            if (defaultChild and isSectionLike(child.type()) and isSectionLike(defaultChild->type()))
                copy = withoutDefaults(static_cast<const Section&>(child),
                    static_cast<const Section&>(*defaultChild));
            else if (child.type() == DomNodeType::Sequence)
                copy = itemsWithoutDefaults(static_cast<const SequenceBase&>(child));
            else if (child.type() == DomNodeType::MultiTypeSequence)
                copy = itemsWithoutDefaults(static_cast<const MultiTypeSequence&>(child));
            else
                copy = child.clone(result.get());

            copy->setParent(result.get());
            result->setChild(key, std::move(copy));
        }
        return result;
    }

}

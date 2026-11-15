module;

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <optional>
#include <span>
#include <vector>

module notecraft.music;

namespace notecraft::music {

namespace {

auto has_id(NoteId id) {
  return [id](const NoteEntry& entry) { return entry.id == id; };
}

void sort_by_note(std::vector<NoteEntry>& entries) {
  std::ranges::sort(entries, {}, &NoteEntry::note);
}

}  // namespace

std::optional<NoteId> Sequence::add(const Note& note) {
  if (!note.is_valid()) {
    return std::nullopt;
  }
  const NoteId id = next_id_++;
  entries_.push_back({id, note});
  return id;
}

bool Sequence::remove(NoteId id) {
  return std::erase_if(entries_, has_id(id)) > 0;
}

bool Sequence::update(NoteId id, const Note& note) {
  if (!note.is_valid()) {
    return false;
  }
  const auto it = std::ranges::find_if(entries_, has_id(id));
  if (it == entries_.end()) {
    return false;
  }
  it->note = note;
  return true;
}

void Sequence::clear() { entries_.clear(); }

std::optional<Note> Sequence::find(NoteId id) const {
  const auto it = std::ranges::find_if(entries_, has_id(id));
  if (it == entries_.end()) {
    return std::nullopt;
  }
  return it->note;
}

std::span<const NoteEntry> Sequence::entries() const { return entries_; }

std::vector<NoteEntry> Sequence::sorted() const {
  std::vector<NoteEntry> result = entries_;
  sort_by_note(result);
  return result;
}

std::vector<NoteEntry> Sequence::notes_between(Tick from, Tick to) const {
  std::vector<NoteEntry> result;
  std::ranges::copy_if(entries_, std::back_inserter(result),
                       [from, to](const NoteEntry& entry) {
                         return entry.note.start < to && entry.note.end() > from;
                       });
  sort_by_note(result);
  return result;
}

std::vector<NoteEntry> Sequence::notes_at(Tick tick) const {
  return notes_between(tick, tick + 1);
}

std::size_t Sequence::size() const { return entries_.size(); }

bool Sequence::empty() const { return entries_.empty(); }

Tick Sequence::length() const {
  Tick result = 0;
  for (const auto& entry : entries_) {
    result = std::max(result, entry.note.end());
  }
  return result;
}

Tempo& Sequence::tempo() { return tempo_; }

const Tempo& Sequence::tempo() const { return tempo_; }

}  // namespace notecraft::music

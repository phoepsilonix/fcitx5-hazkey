#include "hazkey_state.h"

#include <algorithm>
#include <numeric>

#include "hazkey_candidate.h"
#include "hazkey_config.h"

namespace fcitx {

constexpr int NormalCandidateListNBest = 9;
constexpr int PredictCandidateListNBest = 4;

bool HazkeyState::isInputableEvent(const KeyEvent &event) {
    auto key = event.key();
    if (key.check(FcitxKey_space) || key.isSimple() ||
        (key.sym() >= 0x04a1 && key.sym() <= 0x04df)) {
        // 0x04a1 - 0x04dd is the range of kana keys
        return true;
    }
    return false;
}

void HazkeyState::keyEvent(KeyEvent &event) {
    FCITX_DEBUG() << "HazkeyState keyEvent";

    auto candidateList = std::dynamic_pointer_cast<HazkeyCandidateList>(
        event.inputContext()->inputPanel().candidateList());

    if (candidateList != nullptr && candidateList->focused()) {
        candidateKeyEvent(event, candidateList);
    } else if (composingText_ != nullptr) {
        preeditKeyEvent(event, candidateList);
    } else {
        auto key = event.key();

        if (key.check(FcitxKey_space)) {
            // Input full-width space
            // TODO: make this configurable
            ic_->commitString("　");
        } else if (isInputableEvent(event)) {
            // Start composing text and enter preedit mode if key is valid
            composingText_ = kkc_get_composing_text_instance();
            kkc_input_text(composingText_,
                           Key::keySymToUTF8(key.sym()).c_str());
            showPredictCandidateList();
        } else {
            // Pass events to the application in the input state
            return event.filter();
        }
        return event.filterAndAccept();
    }
}

void HazkeyState::preeditKeyEvent(
    KeyEvent &event,
    std::shared_ptr<HazkeyCandidateList> PreeditCandidateList) {
    FCITX_DEBUG() << "HazkeyState preeditKeyEvent";

    auto key = event.key();
    auto keysym = key.sym();

    // TODO: keys should be configurable
    // TODO: use left and right key to move cursor
    switch (keysym) {
        case FcitxKey_Return:
            preedit_.commitPreedit();
            reset();
            break;
        case FcitxKey_BackSpace:
            kkc_delete_backward(composingText_);
            showPredictCandidateList();
            break;
        case FcitxKey_Up:
        case FcitxKey_Down:
        case FcitxKey_Tab:
            PreeditCandidateList->focus(config_->getSelectionKeys());
            updateCandidateCursor(PreeditCandidateList);
            break;
        case FcitxKey_space:
            showNonPredictCandidateList();
            advanceCandidateCursor(
                std::dynamic_pointer_cast<HazkeyCandidateList>(
                    ic_->inputPanel().candidateList()));
            break;
        case FcitxKey_F6:
            directCharactorConversion(ConversionMode::Hiragana);
            break;
        case FcitxKey_F7:
            directCharactorConversion(ConversionMode::KatakanaFullwidth);
            break;
        case FcitxKey_F8:
            directCharactorConversion(ConversionMode::KatakanaHalfwidth);
            break;
        case FcitxKey_F9:
            directCharactorConversion(ConversionMode::RawFullwidth);
            break;
        case FcitxKey_F10:
            directCharactorConversion(ConversionMode::RawHalfwidth);
            break;
        case FcitxKey_Escape:
            reset();
            break;
        case FcitxKey_kana_fullstop:  // kana "。" key
        case FcitxKey_period:
            preedit_.commitPreedit();
            ic_->commitString("。");
            reset();
            break;
        case FcitxKey_kana_conjunctive:  // kana "、" key
        case FcitxKey_comma:
            preedit_.commitPreedit();
            ic_->commitString("、");
            reset();
            break;
        default:
            if (isInputableEvent(event)) {
                kkc_input_text(composingText_,
                               Key::keySymToUTF8(keysym).c_str());
                showPredictCandidateList();
            }
            break;
    }
    return event.filterAndAccept();
}

void HazkeyState::candidateKeyEvent(
    KeyEvent &event, std::shared_ptr<HazkeyCandidateList> candidateList) {
    FCITX_DEBUG() << "HazkeyState candidateKeyEvent";

    auto key = event.key();
    auto keysym = key.sym();

    std::vector<std::string> preedit;
    switch (keysym) {
        case FcitxKey_Right:
            candidateList->next();
            break;
        case FcitxKey_Left:
            candidateList->prev();
            break;
        case FcitxKey_Return: {
            preedit = candidateList->getCandidate(candidateList->cursorIndex())
                          .getPreedit();
            ic_->commitString(preedit[0]);
            if (preedit.size() > 1) {
                auto correspondingCount =
                    candidateList->getCandidate(candidateList->cursorIndex())
                        .correspondingCount();
                kkc_complete_prefix(composingText_, correspondingCount);
                showNonPredictCandidateList();
            } else {
                reset();
            }
        } break;
        case FcitxKey_BackSpace:
            showPredictCandidateList();
            break;
        case FcitxKey_space:
        case FcitxKey_Tab:
            if (key.states() == KeyState::Shift) {
                backCandidateCursor(candidateList);
            } else if (key.states() == KeyState::Alt_Shift) {
                // do nothing
            } else {
                advanceCandidateCursor(candidateList);
            }
            break;
        case FcitxKey_Down:
            advanceCandidateCursor(candidateList);
            break;
        case FcitxKey_Up:
            backCandidateCursor(candidateList);
            break;
        case FcitxKey_F6:
            directCharactorConversion(ConversionMode::Hiragana);
            break;
        case FcitxKey_F7:
            directCharactorConversion(ConversionMode::KatakanaFullwidth);
            break;
        case FcitxKey_F8:
            directCharactorConversion(ConversionMode::KatakanaHalfwidth);
            break;
        case FcitxKey_F9:
            directCharactorConversion(ConversionMode::RawFullwidth);
            break;
        case FcitxKey_F10:
            directCharactorConversion(ConversionMode::RawHalfwidth);
            break;
        case FcitxKey_kana_fullstop:  // kana "。" key
        case FcitxKey_period:
            preedit_.commitPreedit();
            ic_->commitString("。");
            reset();
            break;
        case FcitxKey_kana_conjunctive:  // kana "、" key
        case FcitxKey_comma:
            preedit_.commitPreedit();
            ic_->commitString("、");
            reset();
            break;
        case FcitxKey_Escape:
            reset();
            break;
        default:
            if (key.checkKeyList(config_->getSelectionKeys())) {
                auto localIndex = key.keyListIndex(config_->getSelectionKeys());
                preedit = candidateList->getCandidate(localIndex).getPreedit();
                ic_->commitString(preedit[0]);
                if (preedit.size() > 1) {
                    auto correspondingCount =
                        candidateList->getCandidate(localIndex)
                            .correspondingCount();
                    kkc_complete_prefix(composingText_, correspondingCount);
                    showNonPredictCandidateList();
                } else {
                    reset();
                }
            } else if (isInputableEvent(event)) {
                preedit_.commitPreedit();
                reset();
                composingText_ = kkc_get_composing_text_instance();
                kkc_input_text(composingText_,
                               Key::keySymToUTF8(keysym).c_str());
                showPredictCandidateList();
            } else {
                return event.filter();
            }
            break;
    }
    return event.filterAndAccept();
}

void HazkeyState::directCharactorConversion(ConversionMode mode) {
    char *converted = nullptr;
    switch (mode) {
        case ConversionMode::Hiragana:
            converted = kkc_get_composing_hiragana(composingText_);
            break;
        case ConversionMode::KatakanaFullwidth:
            converted = kkc_get_composing_katakana_fullwidth(composingText_);
            break;
        case ConversionMode::KatakanaHalfwidth:
            converted = kkc_get_composing_katakana_halfwidth(composingText_);
            break;
        case ConversionMode::RawFullwidth:
            converted = kkc_get_composing_alphabet_fullwidth(
                composingText_, preedit_.text().c_str());
            break;
        case ConversionMode::RawHalfwidth:
            converted = kkc_get_composing_alphabet_halfwidth(
                composingText_, preedit_.text().c_str());
            break;
        default:
            return;
    }
    preedit_.setSimplePreedit(converted);
    kkc_free_text(converted);
}

// updatePreedit: whether to update preedit text. currently always true
// nBest: number of candidates to show.
// TODO: make avobe configurable
void HazkeyState::showCandidateList(showCandidateMode mode, int nBest) {
    FCITX_DEBUG() << "HazkeyState showCandidateList";

    bool enabledPreeditConversion =
        mode == showCandidateMode::PredictWithLivePreedit ||
        mode == showCandidateMode::NonPredictWithFirstPreedit;

    bool enabledPredictMode =
        mode == showCandidateMode::PredictWithLivePreedit ||
        mode == showCandidateMode::PredictWithPreedit;

    auto preeditSegmentsPtr = enabledPreeditConversion
                                  ? std::make_shared<std::vector<std::string>>()
                                  : nullptr;

    auto candidates = getCandidates(enabledPredictMode, nBest);

    auto candidateList =
        createCandidateList(std::move(candidates), preeditSegmentsPtr);

    ic_->inputPanel().reset();

    if (enabledPreeditConversion && !preeditSegmentsPtr->empty()) {
        // preedit conversion is enabled and conversion result is found
        // show preedit conversion result
        preedit_.setMultiSegmentPreedit(*preeditSegmentsPtr, 0);
    } else {
        // preedit conversion is disabled or conversion result is not available
        // show hiragana preedit
        auto hiragana = kkc_get_composing_hiragana(composingText_);
        preedit_.setSimplePreedit(hiragana);
        kkc_free_text(hiragana);
    }

    ic_->inputPanel().setCandidateList(std::move(candidateList));
}

std::vector<std::vector<std::string>> HazkeyState::getCandidates(
    bool enabledPreeditConversion, int nBest) {
    auto ***candidates =
        kkc_get_candidates(composingText_, config_->getKkcConfig(),
                           enabledPreeditConversion, nBest);
    std::vector<std::vector<std::string>> candidateList;
    for (int i = 0; candidates[i] != nullptr; i++) {
        std::vector<std::string> candidate;
        for (int j = 0; candidates[i][j] != nullptr; j++) {
            candidate.push_back(candidates[i][j]);
        }
        candidateList.push_back(candidate);
    }
    kkc_free_candidates(candidates);
    return candidateList;
}

std::unique_ptr<HazkeyCandidateList> HazkeyState::createCandidateList(
    std::vector<std::vector<std::string>> candidates,
    std::shared_ptr<std::vector<std::string>> preeditSegments = nullptr) {
    auto candidateList = std::make_unique<HazkeyCandidateList>();
    // for (const auto &candidate : candidates) {
    for (size_t i = 0; i < candidates.size(); i++) {
        auto candidate = candidates[i];
        std::vector<std::string> parts;
        std::vector<int> partLens;

        // collect parts and part lengths
        // more about candidate format, see azookey-kkc/libhazkey.h
        for (size_t j = 5; j < candidate.size() - 1; j += 2) {
            parts.push_back(candidate[j]);
            partLens.push_back(stoi(candidate[j + 1]));
        }
        candidateList->append(std::make_unique<HazkeyCandidateWord>(
            i, candidate[0], candidate[2], stoi(candidate[3]), parts,
            partLens));

        // save preedit which found first
        if (preeditSegments != nullptr && preeditSegments->empty() &&
            stoi(candidate[4]) == 1) {
            *preeditSegments = parts;
        }
    }
    return candidateList;
}

void HazkeyState::showNonPredictCandidateList() {
    if (composingText_ == nullptr) {
        return;
    }

    showCandidateList(showCandidateMode::NonPredictWithFirstPreedit,
                      NormalCandidateListNBest);

    auto newCandidateList = std::dynamic_pointer_cast<HazkeyCandidateList>(
        ic_->inputPanel().candidateList());
    newCandidateList->focus(config_->getSelectionKeys());

    setCandidateCursorAUX(
        std::static_pointer_cast<HazkeyCandidateList>(newCandidateList));
}

void HazkeyState::showPredictCandidateList() {
    if (composingText_ == nullptr) {
        return;
    }
    auto hiragana = kkc_get_composing_hiragana(composingText_);
    if (hiragana == nullptr) {
        reset();
        return;
    }

    showCandidateList(showCandidateMode::PredictWithLivePreedit,
                      PredictCandidateListNBest);

    auto newCandidateList = std::dynamic_pointer_cast<HazkeyCandidateList>(
        ic_->inputPanel().candidateList());
    newCandidateList->setPageSize(PredictCandidateListNBest);

    ic_->inputPanel().setAuxUp(Text("[Tabキーで選択]"));
}

void HazkeyState::completePrefix(int correspondingCount) {
    kkc_complete_prefix(composingText_, correspondingCount);
    // no need for predictions since the conversion is in progress
    showNonPredictCandidateList();
}

void HazkeyState::updateCandidateCursor(
    std::shared_ptr<HazkeyCandidateList> candidateList) {
    setCandidateCursorAUX(candidateList);
    auto text =
        candidateList->getCandidate(candidateList->cursorIndex()).getPreedit();
    preedit_.setMultiSegmentPreedit(text, 0);
}

void HazkeyState::advanceCandidateCursor(
    std::shared_ptr<HazkeyCandidateList> candidateList) {
    candidateList->nextCandidate();
    updateCandidateCursor(candidateList);
}

void HazkeyState::backCandidateCursor(
    std::shared_ptr<HazkeyCandidateList> candidateList) {
    candidateList->prevCandidate();
    updateCandidateCursor(candidateList);
}

void HazkeyState::setCandidateCursorAUX(
    std::shared_ptr<HazkeyCandidateList> candidateList) {
    auto label = "[" + std::to_string(candidateList->globalCursorIndex() + 1) +
                 "/" + std::to_string(candidateList->totalSize()) + "]";
    ic_->inputPanel().setAuxUp(Text(label));
}

void HazkeyState::loadConfig(std::shared_ptr<HazkeyConfig> &config) {
    if (config_ == nullptr) {
        config_ = config;
    }
}

void HazkeyState::reset() {
    FCITX_DEBUG() << "HazkeyState reset";
    if (composingText_ != nullptr) {
        kkc_free_composing_text_instance(composingText_);
    }
    ic_->inputPanel().reset();
    composingText_ = nullptr;
}

}  // namespace fcitx
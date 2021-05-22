#pragma once
#include <array>
#include <memory>       //unique_ptr
#include <type_traits>  //enable_if_t, is_base_of
#include <utility>      //move, forward

#include <QMessageBox>
#include <QString>

namespace message {
using message_t = QMessageBox;
using message_holder = std::unique_ptr<message_t>;

enum class Type {
  Question,
  Info,
  Warning,
  Critical,
};
inline constexpr size_t FACTORY_TYPES_COUNT{4};

class Factory;
using factory_holder = std::unique_ptr<Factory>;
using Plantation = std::array<factory_holder, FACTORY_TYPES_COUNT>;

template <class FactoryTy>
std::enable_if_t<std::is_base_of_v<Factory, FactoryTy>, factory_holder>
MakeFactory() {
  return std::make_unique<FactoryTy>();
}

class Factory {
 public:
  Factory& SetTitle(QString title);
  Factory& SetText(QString text);
  Factory& SetButtons(QMessageBox::StandardButtons buttons);
  virtual message_holder Create() const = 0;
  virtual ~Factory() = default;

 protected:
  template <class... Types>
  static message_holder make_message_helper(Types&&... args) {
    return std::make_unique<message_t>(std::forward<Types>(args)...);
  }

  message_holder make_message(QMessageBox::Icon icon) const {
    return make_message_helper(icon, m_title, m_text, m_buttons);
  }

 protected:
  QString m_text;
  QString m_title;
  QMessageBox::StandardButtons m_buttons;
};

struct Question : public Factory {
  message_holder Create() const override;
};

struct Info : public Factory {
  message_holder Create() const override;
};

struct Warning : public Factory {
  message_holder Create() const override;
};

struct Critical : public Factory {
  message_holder Create() const override;
};
}  // namespace message

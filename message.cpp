#include "message.h"

using namespace std;

namespace message {
Factory& Factory::SetTitle(QString title) {
  m_title = move(title);
  return *this;
}

Factory& Factory::SetText(QString text) {
  m_text = move(text);
  return *this;
}

Factory& Factory::SetButtons(QMessageBox::StandardButtons buttons) {
  m_buttons = buttons;
  return *this;
}

message_holder Question::Create() const {
  return make_message(message_t::Icon::Information);
}

message_holder Info::Create() const {
  return make_message(message_t::Icon::Information);
}

message_holder Warning::Create() const {
  return make_message(message_t::Icon::Warning);
}

message_holder Critical::Create() const {
  return make_message(message_t::Icon::Critical);
}
}  // namespace message

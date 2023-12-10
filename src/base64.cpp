module;

#include <memory>

export module base64;

static char encoding_table[] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
    'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
    'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'};
static int mod_table[] = {0, 2, 1};

export char* base64_encode(const unsigned char* data,
                           size_t data_length,
                           size_t* encoded_length) {
  *encoded_length = 4 * ((data_length + 2) / 3);
  char* encoded_data = reinterpret_cast<char*>(malloc(*encoded_length + 1));
  if (encoded_data == nullptr) {
    return nullptr;
  }
  for (int i = 0, j = 0; i < data_length;) {
    int octet_a = i < data_length ? (unsigned char)data[i++] : 0;
    int octet_b = i < data_length ? (unsigned char)data[i++] : 0;
    int octet_c = i < data_length ? (unsigned char)data[i++] : 0;

    int triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

    encoded_data[j++] = encoding_table[(triple >> 3 * 6) & 0x3F];
    encoded_data[j++] = encoding_table[(triple >> 2 * 6) & 0x3F];
    encoded_data[j++] = encoding_table[(triple >> 1 * 6) & 0x3F];
    encoded_data[j++] = encoding_table[(triple >> 0 * 6) & 0x3F];
  }

  for (int i = 0; i < mod_table[data_length % 3]; i++) {
    encoded_data[*encoded_length - 1 - i] = '=';
  }
  encoded_data[*encoded_length] = 0;

  return encoded_data;
}

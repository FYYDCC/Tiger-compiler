# Lab2

### 各种关键字的处理：

​	就对着注释里面列出的关键字，然后进行一个一个的列举，然后adjust和return。

### 注释的处理：

1. 当遇到 `/*` 开始符号时，`adjust()` 会调整位置并增加 `comment_level_`，然后进入 `StartCondition__::COMMENT` 状态，表示进入注释处理状态。
2. 在 `COMMENT` 状态中，处理注释内容。如果再次遇到 `/*`，则递增 `comment_level_`，表示嵌套了一个新的注释块。如果遇到 `*/`，则递减 `comment_level_`。
3. 当 `comment_level_` 降至 1 时，表示一个嵌套注释块的结束，此时通过 `begin(StartCondition__::INITIAL)` 返回到初始状态，以继续扫描其他代码。
4. `.|\n` 捕获了注释块中的任何字符或换行符，不做处理，只是调用 `adjust()` 调整位置。
4. 注释在文件结束之前没有正确终止，就会执行 `errormsg_->Error(...)`，生成一个错误消息。

### 字符串的处理：

1. 当遇到 `"` 开始符号时，调用 `adjust()` 调整位置，并进入 `StartCondition__::STR` 状态，表示进入字符串处理状态。
2. 在 `STR` 状态中，处理字符串内容。不同的规则匹配不同类型的字符串内容：
   - `\\n` 匹配换行符，将 `\n` 添加到 `string_buf_` 中。
   - `\\t` 匹配制表符，将 `\t` 添加到 `string_buf_` 中。
   - `\"` 匹配引号，表示字符串结束。将 `string_buf_` 中的内容设置为匹配的字符串，清空 `string_buf_`，然后返回到初始状态 `StartCondition__::INITIAL`，并将解析器的当前标记设置为字符串类型 `Parser::STRING`。
   - `\\\"` 匹配双引号字符 `\"`，将其添加到 `string_buf_` 中。
   - `\\\\"` 匹配反斜杠字符 `\\`，将其添加到 `string_buf_` 中。
   - `\\"[[:digit:]]{3}` 匹配八进制字符转义，将其添加到 `string_buf_` 中，这里使用 `atoi` 将匹配的数字字符转换为相应的字符。
   - `\\"[[:space:]]+\\"` 匹配空白字符转义，不做处理。
   - `\\^"[A-Z]` 匹配控制字符转义，将对应的 ASCII 控制字符添加到 `string_buf_` 中。
   - `.` 匹配其他字符，将其添加到 `string_buf_` 中。


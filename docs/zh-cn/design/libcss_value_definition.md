# libcss

CSS 解析库。

## 需求

添加新的 CSS 属性注册函数，支持使用 [CSS属性值定义语法
](https://developer.mozilla.org/en-US/docs/Web/CSS/Value_definition_syntax) 定义属性的值。

## 分析

先挑几个典型的 CSS 值定义例子：

- color: `<color>`
- font-size: `<absolute-size> | <relative-size> | <length-percentage>`
- position: `static | relative | absolute | sticky | fixed`
- border: `<line-width> || <line-style> || <color>`
- box-shadow: `none | <shadow>`
- width: `auto | <length> | <percentage> | min-content | max-content | fit-content | fit-content(<length-percentage>)`

像 position 这种由多个关键字组成的定义，最简单的方式是解析成数组然后遍历数组逐个判断，但对于 box-shadow 这种有自定义数据类型的就不好做了，box-shadow 值的完整定义是这样的：

```
none | <shadow>#
where
<shadow> = inset? && <length>{2,4} && <color>?
```

如果把 box-shadow 值的匹配过程看成一颗树，每个可选值按存在与否分出一个分支，那么这颗树就是这样的：

```
- none
- <shadow>
  - inset?
    - inset && <length>{2,4} && <color>?
        - inset && <length>{2,4} && <color>
        - inset && <length>{2,4}
    - <length>{2,4} && <color>?
        - <length>{2,4} && <color>
        - <length>{2,4}
```

从中可以看出，CSS 值的类型定义数据适合存储为树形结构，而值的匹配过程就是树的遍历过程。

## 设计

CSS 值定义模块要做的事情有三个：解析包含值定义代码的字符串、存储解析结果、匹配对应的值定义，那么需要设计的就是数据结构、解析器和匹配器。

### 数据结构

CSS 值定义的数据结构应表达以下内容：

- 匹配模式：`||` `&&` `<>` `|` `?`
- 类型：`<percentage>`
- 别名：`<length-percentage> where length-percentage = <length> | <percentage>`

### 解析器

### 匹配器

匹配器需要解决的问题如下：

- **如何确定每个值的边界？**

  每个值都以空白符为终点，在判定终点时需要考虑到被单引号或双引号的字符串，例如：`"Microsoft YaHei"`。

  虽然预先分割所有值是个简单直接的做法，但它需要更多的读写操作、内存分配和释放操作，所以出于性能上的考虑，应使用两个变量来记录值的起始和结束位置。

- **如何匹配值？**

  针对不同类型的值定义来分别处理。

- **如何切换到下一个值？**

  从值的终点开始遍历查找下个值的起点和终点，然后传给匹配函数。

- **什么情况下切换到下一个值？**

  - DOUBLE BAR
  - DOUBLE AMPERSAND
  - JUXTAPOSITION

  需要注意的是，必须在开始解析下个值之前进行切换而不是每次解析完后切换，否则多余的切换会导致整个解析结果错误。

  ```diff
  + if (i > 0) {
  +     css_value_matcher_resolve_next_value(matcher);
  + }
    if (css_value_matcher_match(matcher, node->data) != 0) {
        return -1;
    }
  - css_value_matcher_resolve_next_value(matcher);
  + i++;
  ```

- **匹配失败时如何切换到下个规则？**

  匹配失败时返回失败值，上级函数靠判断该值来决定是否使用下个规则。

- **如何存储已匹配的值？**

  将已匹配的值存为数组，然后给匹配函数增加一个用于记录下标的参数。

结合上述问题解决方案，可得出如下数据结构：

```c
struct css_value_matching_context_t {
  const char *value_str;
  unsigned value_str_len;

  css_style_value_t value;
  unsigned index;
};
```

匹配过程的伪代码如下：

```c
```

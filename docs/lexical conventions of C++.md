## C++ lexical conventions(specific to C++17)

### namespace

![image-20230906170518239](https://gallery-1259614029.cos.ap-chengdu.myqcloud.com/img/image-20230906170518239.png)

```cpp
(1)
namespace aa {

  static const int a = 3;
  void dosomething();
}

(2)
inline namespace bb {

  static const int a = 3;
  void dosomething();
}

(3)
namespace {

  static const int a = 3;
  void dosomething();
}

(8)
namespace A::B::C {

  static const int a = 3;
  void dosomething();

}
```

### enumeration

![image-20230906170702445](https://gallery-1259614029.cos.ap-chengdu.myqcloud.com/img/image-20230906170702445.png)

```cpp
// unscoped
enum {

};

// scoped
enum class Week{
  MON,
  TUE,
  WED,
};

// scoped noname
enum struct {

};

// scoped with base
enum class K: base {

};


```

### using 

(1)

![image-20230907144831071](https://gallery-1259614029.cos.ap-chengdu.myqcloud.com/img/image-20230907144831071.png)

![image-20230907144856235](https://gallery-1259614029.cos.ap-chengdu.myqcloud.com/img/image-20230907144856235.png)

(2)

![image-20230907144928112](https://gallery-1259614029.cos.ap-chengdu.myqcloud.com/img/image-20230907144928112.png)

![image-20230907145000285](https://gallery-1259614029.cos.ap-chengdu.myqcloud.com/img/image-20230907145000285.png)

(3) not supported, this is C++ 20 and afterwards

![image-20230907145038641](https://gallery-1259614029.cos.ap-chengdu.myqcloud.com/img/image-20230907145038641.png)

(4)

![image-20230907145236460](https://gallery-1259614029.cos.ap-chengdu.myqcloud.com/img/image-20230907145236460.png)

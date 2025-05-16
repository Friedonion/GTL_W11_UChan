# GTL_W10_T8

# Clone시 주의사항
이 레포지토리에는 submodule이 포함되어있어, 일반적인 방식으로 clone하면, 컴파일할 때 정상적으로 컴파일되지 않습니다.

> 다음 명령어를 사용해서 Clone해 주세요
```shell
git clone --recursive <repository_url>
```

> 이미 Clone을 했다면 다음 명령어를 추가로 입력해 주세요
```shell
git submodule update --init --recursive
```

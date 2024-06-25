
//
// Created by fanshuhua on 2024/5/31.
//

#include "AOP.h"


struct LoggingAspect {
    template<typename... Ts>
    static void Before(Ts... ars) {
        cout << "entering" << endl;
    }

    template<typename... Ts>
    static void After(Ts... args) {
//        展开参数包
        int arr[] = {args...};
        for (auto i: arr) {
            cout << i << endl;
        }
        cout << "leaving" << endl;
    }
};

void funcTest(int i, char a) {
    cout << "funcTes1t" << endl;
}

int main() {
    Invoke<LoggingAspect>(&funcTest, 1, 'a');
    return 0;
}
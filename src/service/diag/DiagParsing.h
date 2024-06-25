//
// Created by 87837 on 2024/6/11.
//

#ifndef DLLTEST_DIAGPARSING_H
#define DLLTEST_DIAGPARSING_H

#include "../../model/vo/DiagV0.h"

class ParsingChain {
public:
    virtual cclCanMessage *parse(DiagSession *parsingDTO, DiagConfig *diagConfig) = 0;

    virtual bool isSupport(DiagSession *parsingDTO, DiagConfig *diagConfig) = 0;
};

class SF_Parsing : public ParsingChain {
public:
    cclCanMessage *parse(DiagSession *parsingDTO, DiagConfig *diagConfig) override;

    bool isSupport(DiagSession *parsingDTO, DiagConfig *diagConfig) override;
};

class FF_Parsing : public ParsingChain {
public:
    cclCanMessage *parse(DiagSession *parsingDTO, DiagConfig *diagConfig) override;

    bool isSupport(DiagSession *parsingDTO, DiagConfig *diagConfig) override;
};

class CF_Parsing : public ParsingChain {
public:
    cclCanMessage *parse(DiagSession *parsingDTO, DiagConfig *diagConfig) override;

    bool isSupport(DiagSession *parsingDTO, DiagConfig *diagConfig) override;
};

class ParsingFactory {
private:
    std::vector<ParsingChain *> parsingChainList;

    ParsingFactory() {
        parsingChainList.push_back(new SF_Parsing());
        parsingChainList.push_back(new FF_Parsing());
        parsingChainList.push_back(new CF_Parsing());
    }

public:
    static ParsingFactory *getInstance() {
        static ParsingFactory *instance = nullptr;
        if (instance == nullptr) {
            instance = new ParsingFactory();
        }
        return instance;
    }

    cclCanMessage *parse(DiagSession *parsingDTO, DiagConfig *diagConfig) {
        for (auto parsingChain: parsingChainList) {
            if (parsingChain->isSupport(parsingDTO, diagConfig)) {
                return parsingChain->parse(parsingDTO, diagConfig);
            }
        }
        return {};
    }
};


#endif //DLLTEST_DIAGPARSING_H

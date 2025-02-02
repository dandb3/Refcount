#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/JSONCompilationDatabase.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "llvm/Support/CommandLine.h"

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <linux/limits.h>
#include <fstream>
#include <iostream>
#include <iomanip>

#define LOG_DIR "/home/jdoh/test/refcount_count/log/"
#define COMPILE_DATABASE "/home/jdoh/test/refcount_count/compile_commands.json"

using namespace llvm;
using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::tooling;

// ----------------------------------------------------------------------------
// COMMAND LINE ARGUMENT SETUP
// ----------------------------------------------------------------------------

// Set up the standard command line arguments for the tool.
// These command line arguments are standard across all tools
// built using the LLVM framework.
static cl::OptionCategory refcntCategory("refcnt options");
static cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);

// // Here we add an extra non-standard command line flag 
// // for demonstration purposes
static cl::opt<bool> verbose("verbose",
    cl::desc(R"(Generate verbose output)"),     // the description
    cl::init(false),                            // the initial value of the option
    cl::cat(refcntCategory)                   // what category this belongs to
);

// ----------------------------------------------------------------------------
// DEFAULT WARNING SUPPRESSION
// ----------------------------------------------------------------------------

// The WarningDiagConsumer allows us to suppress warning and error messages
// which are raised when a file is being parsed by clang. This allows us to
// turn off extraneous output since we assume that we are checking code 
// which already compiles.
class WarningDiagConsumer : public DiagnosticConsumer {

    public:
    virtual void HandleDiagnostic(
            DiagnosticsEngine::Level Level, const Diagnostic& Info) override {
        // simply do nothing
    }
};

// ----------------------------------------------------------------------------
// CALLBACK CLASSES
// ----------------------------------------------------------------------------

// Here we will be adding our callback classes which will be
// doing the actual code analysis
//
// ...
class Refcnt {
    public:
    int atomic_t_cnt;
    int atomic_long_t_cnt;
    int atomic64_t_cnt;
    int refcount_t_cnt;
    int kref_cnt;
    
    Refcnt()
    : atomic_t_cnt(0), atomic_long_t_cnt(0), atomic64_t_cnt(0),
        refcount_t_cnt(0), kref_cnt(0)
    {}

    Refcnt &operator+=(const Refcnt &refcnt) {
        atomic_t_cnt += refcnt.atomic_t_cnt;
        atomic_long_t_cnt += refcnt.atomic_long_t_cnt;
        atomic64_t_cnt += refcnt.atomic64_t_cnt;
        refcount_t_cnt += refcnt.refcount_t_cnt;
        kref_cnt += refcnt.kref_cnt;
        return *this;
    }
};

static std::ofstream total_output;
static Refcnt total_refcnt;

class TypeCheck : public MatchFinder::MatchCallback {
    private:
    std::map<std::string, std::pair<std::stringstream, Refcnt>> files;

    public:
    virtual void onStartOfTranslationUnit() override {
        
    }

    virtual void onEndOfTranslationUnit() override {
        std::string logFileDir;
        std::ofstream ofs;

        for (auto &elem : files) {
            logFileDir = elem.first.substr(0, elem.first.rfind('/'));
            if (access(logFileDir.c_str(), F_OK) != 0) {
                system(("mkdir -p " + logFileDir).c_str());
            }
            
            ofs.open(elem.first);
            if (!ofs.is_open()) {
                llvm::errs() << "Log file " << elem.first << " open failed!\n";
                exit(1);
            }
            ofs << elem.second.first.str();
            ofs << "atomic_t: " << elem.second.second.atomic_t_cnt << "\n"
                << "atomic_long_t: " << elem.second.second.atomic_long_t_cnt << "\n"
                << "atomic64_t: " << elem.second.second.atomic64_t_cnt << "\n"
                << "refcount_t: " << elem.second.second.refcount_t_cnt << "\n"
                << "kref: " << elem.second.second.kref_cnt << "\n";
            ofs.close();
            total_refcnt += elem.second.second;
        }
    }

    virtual void run(const MatchFinder::MatchResult& Result) override {
        const clang::DeclaratorDecl *node = Result.Nodes.getNodeAs<clang::DeclaratorDecl>("refcntType");

        if (node == nullptr) {
            return;
        }
        
        const auto &SM = *Result.SourceManager;
        const auto &loc = node->getBeginLoc();
        const auto &srcFile = SM.getFilename(SM.getSpellingLoc(loc)).str();
        std::string logFile;

        if (srcFile.empty()) {
            llvm::errs() << "Path empty!\n";
            return;
        }
        logFile = LOG_DIR + srcFile;
        // llvm::outs() << "srcFile: " << srcFile << "\n";
        // llvm::outs() << "logFile: " << logFile << "\n";

        if (access(logFile.c_str(), F_OK) == 0) {
            return;
        }

        auto &pair = files.insert({logFile, std::pair<std::stringstream, Refcnt>()}).first->second;
        const std::string &type = node->getType().getAsString();
        
        if (type == "atomic_t") {
            ++pair.second.atomic_t_cnt;
        } else if (type == "atomic_long_t") {
            ++pair.second.atomic_long_t_cnt;
        } else if (type == "atomic64_t") {
            ++pair.second.atomic64_t_cnt;
        } else if (type == "refcount_t") {
            ++pair.second.refcount_t_cnt;
        } else if (type == "struct kref") {
            ++pair.second.kref_cnt;
        }

        std::string rowcol = std::to_string(SM.getExpansionLineNumber(loc)) + ":" + std::to_string(SM.getExpansionColumnNumber(loc));

        pair.first << std::left << std::setw(10) << rowcol
                   << "Name: " << std::setw(20) << node->getName().str()
                   << "Type: " << node->getType().getAsString() << "\n";
    }
};

// class IncludeRewriteCallback : public clang::PPCallbacks {
//     public:
//     virtual void InclusionDirective(
//         SourceLocation HashLoc,
//         const Token &IncludeTok, StringRef FileName,
//         bool IsAngled, CharSourceRange FilenameRange,
//         OptionalFileEntryRef File,
//         StringRef SearchPath, StringRef RelativePath,
//         const Module *SuggestedModule,
//         bool ModuleImported,
//         SrcMgr::CharacteristicKind FileType) override {
        
//     }

//     private:
//     Rewriter rewriter;
//     const SourceManager &SM;
// };

// ----------------------------------------------------------------------------
// REGISTERING CALLBACKS
// ----------------------------------------------------------------------------

// The ASTConsumer allows us to dictate:
//      - which AST nodes we match, and
//      - how we want to handle those matched nodes
class RefcntASTConsumer : public ASTConsumer {

    public:
    RefcntASTConsumer(clang::Preprocessor& PP) {
    
        // Here we add all of the checks that should be run
        // when the AST is traversed by using Matcher.addMatcher

        // At the moment, there are no checks registered

        // PP.addPPCallbacks(std::make_unique<clang::PPCallbacks>());

        auto *callback = new TypeCheck;

        Matcher.addMatcher(
            fieldDecl(
                anyOf(
                    hasType(typedefNameDecl(hasAnyName(
                        "atomic_t",
                        "atomic_long_t",
                        "atomic64_t",
                        "refcount_t"
                    ))),
                    hasType(recordDecl(hasName("kref")))
                ),
                unless(hasAncestor(recordDecl(hasAnyName(
                    "kref",
                    "refcount_t"
                ))))
            ).bind("refcntType"),
            callback
        );
        // Matcher.addMatcher(
        //     declaratorDecl(
        //         matchesName(".*ref.*"),
        //         unless(anyOf(
        //             hasType(typedefNameDecl(hasAnyName(
        //                 "atomic_t",
        //                 "atomic_long_t",
        //                 "atomic64_t",
        //                 "refcount_t"
        //             ))),
        //             hasType(recordDecl(hasName("kref")))
        //         )),
        //         anyOf(
        //             varDecl(),
        //             fieldDecl()
        //         )
        //     ).bind("refcntName"),
        //     callback
        // );
    }

    void HandleTranslationUnit(ASTContext& Context) override {
        Matcher.matchAST(Context);
    }

    private:
    MatchFinder Matcher;
};

// The FrontEndAction is the main entry point for the clang tooling library
// and allows us to add callbacks via:
//      PPCallback classes  - for callbacks involving the preprocessor
//      ASTConsumer classes - for callbacks involving AST nodes 
class RefcntFrontEndAction : public ASTFrontendAction {

    public:
    virtual bool BeginSourceFileAction(CompilerInstance &CI) override {
        const auto &SM = CI.getSourceManager();
        
        // const std::string &filePath = SM.getFileEntryForID(SM.getMainFileID())->tryGetRealPathName().str();

        // llvm::outs() << "File path: " << filePath << "\n";

        return true;
    }

    virtual std::unique_ptr<ASTConsumer> CreateASTConsumer(
            CompilerInstance& CI, StringRef file) override {

        // Here we add any checks which require the preprocessor.
        // At the moment, there are no checks registered

        return std::unique_ptr<ASTConsumer>(
                new RefcntASTConsumer(CI.getPreprocessor()));
    }

    virtual void EndSourceFileAction() override {
        
    }

    // virtual bool ParseArgs(
    //     const CompilerInstance &CI,
    //     const std::vector<std::string> &Args
    // ) override {
    //     return true;
    // }
};

// static FrontendPluginRegistry::Add<RefcntFrontEndAction> X("refcnt-plugin", "find refcnt");

// ----------------------------------------------------------------------------
// OUR PROGRAM
// ----------------------------------------------------------------------------

// Checks if the given filepath can be accessed without issue.
// Returns true if it is accessible, else returns false.
bool filepathAccessible(std::string path)
{   
    std::ifstream file(path);
    return file.is_open();
}

int main(int argc, const char** argv)
{
    // Parse the command line arguments. This will provide us with
    // the list of files we need to check as well as allow us to
    // check for the optional flags. Note that we also allow for
    // zero or more arguments to allow for more fine-grained error
    // checking
    if (argc > 1) {
        auto OptionsParser = CommonOptionsParser::create(argc, argv, refcntCategory, cl::ZeroOrMore);
        if (auto err = OptionsParser.takeError()) {
            llvm::errs() << std::move(err);
            return EXIT_FAILURE;
        }

        // Our program is meant to analyse source code, so if we didn't
        // get any filepaths, we print an error message and exit
        auto files = OptionsParser->getSourcePathList();
        if (files.empty()) {
            llvm::errs() << "Error: No input files specified\n";
            return EXIT_FAILURE;
        }

        // Also, if any of the filepaths we've received are invalid,
        // we also print an error message and exit
        for (auto path : files) {
            if (!filepathAccessible(path)) {
                llvm::errs() << "Unable to access file '" << path << "'\n";
                return EXIT_FAILURE;
            }
        }
        ClangTool Tool(OptionsParser->getCompilations(), files);
        Tool.setDiagnosticConsumer(new WarningDiagConsumer);
        Tool.run(newFrontendActionFactory<RefcntFrontEndAction>().get());
    }
    else {
        std::string err_msg;
        auto database = clang::tooling::JSONCompilationDatabase::loadFromFile(COMPILE_DATABASE, err_msg, JSONCommandLineSyntax::AutoDetect);
        for (std::string& path : database->getAllFiles()) {
            if (!filepathAccessible(path)) {
                llvm::errs() << "Unable to access file '" << path << "'\n";
            }
        }

        // Next, we create the tool which will perform all of the 
        // code analysis. In the base code you are provided, this
        // tool doesn't perform any analysis at all.
        ClangTool Tool(*database, database->getAllFiles());
        Tool.setDiagnosticConsumer(new WarningDiagConsumer);
        Tool.run(newFrontendActionFactory<RefcntFrontEndAction>().get());
    }

    llvm::outs() << "atomic_t: " << total_refcnt.atomic_t_cnt << "\n"
                 << "atomic_long_t: " << total_refcnt.atomic_long_t_cnt << "\n"
                 << "atomic64_t: " << total_refcnt.atomic64_t_cnt << "\n"
                 << "refcount_t: " << total_refcnt.refcount_t_cnt << "\n"
                 << "kref: " << total_refcnt.kref_cnt << "\n";

    total_output.open(LOG_DIR + std::string("log.txt"));
    if (!total_output.is_open()) {
        llvm::errs() << "output file open failed!\n";
        return EXIT_FAILURE;
    }

    total_output << "atomic_t: " << total_refcnt.atomic_t_cnt << "\n"
                 << "atomic_long_t: " << total_refcnt.atomic_long_t_cnt << "\n"
                 << "atomic64_t: " << total_refcnt.atomic64_t_cnt << "\n"
                 << "refcount_t: " << total_refcnt.refcount_t_cnt << "\n"
                 << "kref: " << total_refcnt.kref_cnt << "\n";

    return EXIT_SUCCESS;
}

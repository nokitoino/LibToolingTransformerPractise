// Declares clang::SyntaxOnlyAction.
#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
// Declares llvm::cl::extrahelp.
#include "llvm/Support/CommandLine.h"

// Access Matched Nodes Information
#include "clang/AST/ASTContext.h"

using namespace clang::tooling;
using namespace llvm;
// Apply a custom category to all command-line options so that they are the
// only ones displayed.
static llvm::cl::OptionCategory MyToolCategory("my-tool options");

// CommonOptionsParser declares HelpMessage with a description of the common
// command-line options related to the compilation database and input files.
// It's nice to have this help message in all tools.
static cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);

// A help message for this specific tool can be added afterwards.
static cl::extrahelp MoreHelp("\nMore help text...\n");
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"


#include "clang/Tooling/Transformer/Transformer.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Tooling/Transformer/RangeSelector.h"
#include "clang/Tooling/Transformer/RewriteRule.h"
#include "clang/Tooling/Transformer/Stencil.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/Errc.h"
#include "llvm/Support/Error.h"



#include "llvm/Support/CommandLine.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Signals.h"
#include "llvm/Support/raw_ostream.h"


#include "clang/Frontend/CommandLineSourceLoc.h"
#include "clang/Frontend/TextDiagnosticPrinter.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Refactoring.h"
#include "clang/Tooling/Refactoring/RefactoringAction.h"
#include "clang/Tooling/Refactoring/RefactoringOptions.h"
#include "clang/Tooling/Refactoring/Rename/RenamingAction.h"

using namespace clang;
using namespace clang::ast_matchers;
using namespace transformer;

namespace cl = llvm::cl;

namespace opts {

static cl::OptionCategory CommonRefactorOptions("Refactoring options");

static cl::opt<bool> Verbose("v", cl::desc("Use verbose output"),
                             cl::cat(cl::getGeneralCategory()),
                             cl::sub(cl::SubCommand::getAll()));

static cl::opt<bool> Inplace("i", cl::desc("Inplace edit <file>s"),
                             cl::cat(cl::getGeneralCategory()),
                             cl::sub(cl::SubCommand::getAll()));

} // end namespace opts

StatementMatcher LoopMatcher =
  forStmt().bind("forLoop");

class LoopPrinter : public MatchFinder::MatchCallback {
public :
  virtual void run(const MatchFinder::MatchResult &Result) override {
    if (const ForStmt *FS = Result.Nodes.getNodeAs<clang::ForStmt>("forLoop"))
      FS->dump();
  }
};

  std::vector<std::unique_ptr<Transformer>> Transformers;
  MatchFinder Finder;
  AtomicChanges Changes;
  std::vector<std::string> StringMetadata;
 Transformer::ChangeSetConsumer consumer() {
    return [&](Expected<MutableArrayRef<AtomicChange>> C) {
      if (C) {
        Changes.insert(Changes.end(), std::make_move_iterator(C->begin()),
                       std::make_move_iterator(C->end()));
      } else {
        // FIXME: stash this error rather than printing.
        llvm::errs() << "Error generating changes: "
                     << llvm::toString(C.takeError()) << "\n";
      }
    };
  }
auto consumerWithStringMetadata() {
    return [&](Expected<TransformerResult<std::string>> C) {
      if (C) {
        Changes.insert(Changes.end(),
                       std::make_move_iterator(C->Changes.begin()),
                       std::make_move_iterator(C->Changes.end()));
        StringMetadata.push_back(std::move(C->Metadata));
      } else {
        // FIXME: stash this error rather than printing.
        llvm::errs() << "Error generating changes: "
                     << llvm::toString(C.takeError()) << "\n";
      }
    };
  }
 void testRule(RewriteRuleWith<std::string> Rule) {
    Transformers.push_back(std::make_unique<Transformer>(std::move(Rule), consumerWithStringMetadata()));
    Transformers.back()->registerMatchers(&Finder);
  }
void testRule(RewriteRule Rule){
    Transformers.push_back(
        std::make_unique<Transformer>(std::move(Rule), consumer()));
    Transformers.back()->registerMatchers(&Finder);  
}


bool applySourceChanges() {
    std::set<std::string> Files;
    for (const auto &Change : Changes){
      Files.insert(Change.getFilePath());
      llvm::outs() << Change.getFilePath();
    }
    // FIXME: Add automatic formatting support as well.
    tooling::ApplyChangesSpec Spec;
    // FIXME: We should probably cleanup the result by default as well.
    Spec.Cleanup = false;
    for (const auto &File : Files) {
      llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>> BufferErr =
          llvm::MemoryBuffer::getFile(File);
      if (!BufferErr) {
        llvm::errs() << "error: failed to open " << File << " for rewriting\n";
        return true;
      }
      auto Result = tooling::applyAtomicChanges(File, (*BufferErr)->getBuffer(),
                                                Changes, Spec);
      if (!Result) {
        llvm::errs() << toString(Result.takeError());
        return true;
      }

      if (opts::Inplace) {
        std::error_code EC;
        llvm::raw_fd_ostream OS(File, EC, llvm::sys::fs::OF_TextWithCRLF);
        if (EC) {
          llvm::errs() << EC.message() << "\n";
          return true;
        }
        OS << *Result;
        continue;
      }

      llvm::outs() << *Result;
    }
    return false;
  }

int main(int argc, const char **argv) {
  auto ExpectedParser = CommonOptionsParser::create(argc, argv, MyToolCategory);
  if (!ExpectedParser) {
    // Fail gracefully for unsupported options.
    llvm::errs() << ExpectedParser.takeError();
    return 1;
  }

  CommonOptionsParser& OptionsParser = ExpectedParser.get();
  ClangTool Tool(OptionsParser.getCompilations(),
                 OptionsParser.getSourcePathList());

  
   auto Rule = makeRule(functionDecl(hasName("MkX")),
         changeTo(cat("MakeX")),
         cat("MkX has been renamed MakeX"));
 /*RewriteRule Rule = makeRule(declRefExpr(to(functionDecl(hasName("MkX")))),
         changeTo(cat("MakeX")),
         cat("MkX has been renamed MakeX"));
 */
  LoopPrinter Printer;
  Finder.addMatcher(LoopMatcher, &Printer);
  testRule(std::move(Rule));

  Tool.run(newFrontendActionFactory(&Finder).get());
  applySourceChanges();
  return 0;
}

import System.Environment
import Data.List
import Text.ParserCombinators.Parsec

---------------------------

data GenDefinition = ComponentDefinition String [GenEntryDefinition]
                   | SystemDefinition String [GenEntryDefinition]
                   deriving(Show, Eq)

data GenEntryDefinition = MemberList [VariableDeclaration]
                        | FunctionList [FunctionEntry]
                        | Family [FamilyComponentEntry]
                        | Option String String
                        deriving(Show, Eq)
                        
type VariableDeclaration = (String, String)
type FunctionSignature = ([VariableDeclaration], String)
type FunctionEntry = (String, FunctionSignature)
type FamilyComponentEntry = String


---------------------------


nonBreakingChars = " \t"
newLineChars = "\n\r"
whiteSpaceChars = nonBreakingChars ++ newLineChars
reservedChars = "{}:=(),"

whiteSpace = many (oneOf whiteSpaceChars)
nonBreakingWhiteSpace = many (oneOf nonBreakingChars)

eol =   try (string "\n\r")
    <|> try (string "\r\n")
    <|> string "\n"
    <|> string "\r"
    <?> "end of line"

-- Matches whitespace, but must contain at least one EOF    
whiteSpaceWithEol = do
    nonBreakingWhiteSpace
    eol
    whiteSpace
    
-- Trims whitespace around p
trim p = try (do
    whiteSpace
    p <* whiteSpace)

    
---------------------------


identifier = many1 (noneOf (whiteSpaceChars ++ reservedChars)) <?> "identifier"
typeName = many1 (noneOf (newLineChars ++ reservedChars)) <?> "type name"

outerEntry typeName c f = do
    string typeName
    genName <- trim (identifier)
    trim (char '=')
    contents <- block c
    return $ f genName contents

innerEntry typeName p = do
    string typeName
    trim (char '=')
    p

block c = char '{' >> (entrySequence c) <* (char '}')
entrySequence c = whiteSpace >> many (c <* whiteSpaceWithEol)

variable = do
    name <- identifier
    trim (char ':')
    varType <- typeName
    return (name, varType)
    
function = do
    name <- identifier
    trim (char ':')
    signature <- functionSignature
    return (name, signature)

functionSignature = do
    char '('
    args <- sepBy (trim variable) (char ',')
    char ')'
    trim (string "->")
    returnType <- typeName
    return (args, returnType)
    
memberList = innerEntry "members" (do { block <- block variable ; return $ MemberList block })
functionList = innerEntry "functions" (do { block <- block function ; return $ FunctionList block })
familyList = innerEntry "family" (do { block <- block typeName ; return $ Family block })
specificOption n = innerEntry n (do { opt <- identifier; return $ Option n opt })

optionEntry = do
    try (specificOption "strategy")
    <|> try (specificOption "access")
    <|> try (specificOption "language")
    <|> try (specificOption "time")
    <|> specificOption "subDivide"

componentEntries = do
    memberList
    <|> functionList

systemEntries = do
    familyList
    <|> optionEntry

componentDef = outerEntry "component" (componentEntries) (\n ges -> ComponentDefinition n ges)
systemDef = outerEntry "system" (systemEntries) (\n ges -> SystemDefinition n ges)

genDef = do
    try componentDef
    <|> systemDef

genDefs = (entrySequence genDef) <* eof

parseFile :: String -> Either ParseError [GenDefinition]
parseFile input = parse genDefs "(unknown)" input


---------------------------


main = do
    args <- getArgs
    rawFile <- readFile (head args)
    putStrLn $ case parseFile rawFile of
        Left error -> "Error: " ++ (show error)
        Right defs -> show defs
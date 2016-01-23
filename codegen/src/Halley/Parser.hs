{-
   Copyright 2016 Rodrigo Braz Monteiro

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
-}
module Halley.Parser
( parseFile
) where

import Halley.AST
import Text.ParserCombinators.Parsec
import Data.List


--------------------------


parseFile :: String -> Either ParseError [GenDefinition]
parseFile input = parse genDefs "(unknown)" input


-------------------------

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
    nonBreakingWhiteSpace
    p <* nonBreakingWhiteSpace)



-------------------------


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
